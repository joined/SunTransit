#include <chrono>
#include <esp_http_client.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <esp_wifi.h>
#include <freertos/event_groups.h>
#include <lwip/apps/netbiosns.h>
#include <mdns.h>
#include <sys/param.h>
#include <thread>
#include <unordered_set>
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_softap.h>

#include "bvg_api_client.hpp"
#include "http_server.hpp"
#include "lcd.hpp"
#include "nvs_engine.hpp"
#include "time.hpp"
#include "ui.hpp"
#include "utils.hpp"

/* Signal Wi-Fi events on this event-group */
const int WIFI_CONNECTED_EVENT = BIT0;
static EventGroupHandle_t wifi_event_group;

static const char *TAG = "MAIN";
static esp_timer_handle_t wifi_timeout_timer = nullptr;

#define PROV_MGR_MAX_RETRY_COUNT 3

using namespace std::chrono_literals;

static constexpr auto DEPARTURES_REFRESH_PERIOD = 10s;
static constexpr auto LAST_UPDATED_REFRESH_PERIOD = 500ms;

void reset_wifi_and_reboot() {
    ESP_LOGI(TAG, "WiFi reset requested by user");
    ESP_LOGI(TAG, "Resetting WiFi provisioning and rebooting...");

    // Reset WiFi provisioning
    ESP_ERROR_CHECK(wifi_prov_mgr_reset_provisioning());

    // Small delay to ensure the reset is processed
    std::this_thread::sleep_for(1s);

    // Reboot the device
    esp_restart();
}

static void provisioning_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    static int retries;
    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "Provisioning started");
            break;
        case WIFI_PROV_CRED_RECV: {
            wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
            ESP_LOGI(TAG,
                     "Received Wi-Fi credentials"
                     "\n\tSSID     : %s\n\tPassword : %s",
                     (const char *)wifi_sta_cfg->ssid, (const char *)wifi_sta_cfg->password);
            break;
        }
        case WIFI_PROV_CRED_FAIL: {
            wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
            ESP_LOGE(TAG,
                     "Provisioning failed!\n\tReason : %s"
                     "\n\tPlease reset to factory and retry provisioning",
                     (*reason == WIFI_PROV_STA_AUTH_ERROR) ? "Wi-Fi station authentication failed"
                                                           : "Wi-Fi access-point not found");
            retries++;
            if (retries >= PROV_MGR_MAX_RETRY_COUNT) {
                ESP_LOGI(TAG, "Failed to connect with provisioned AP, reseting provisioned credentials");
                wifi_prov_mgr_reset_sm_state_on_failure();
                retries = 0;
            }
            break;
        }
        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG, "Provisioning successful");
            retries = 0;
            break;
        case WIFI_PROV_END:
            /* De-initialize manager once provisioning is finished */
            wifi_prov_mgr_deinit();
            break;
        default:
            break;
        }
    } else if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
            esp_wifi_connect();
            break;
        case WIFI_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG, "SoftAP transport: Connected!");
            break;
        case WIFI_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(TAG, "SoftAP transport: Disconnected!");
            break;
        default:
            break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));

        /* Stop the WiFi timeout timer if it's running */
        if (wifi_timeout_timer && esp_timer_is_active(wifi_timeout_timer)) {
            esp_timer_stop(wifi_timeout_timer);
        }

        /* Signal main application to continue execution */
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_EVENT);
    }
}

static void init_mdns_and_netbios(void) {
    // TODO Handle collision problem, idea: do a query before setting the hostname and check if someone else
    // on the network is already using the default hostname (`suntransit.local`). If so, append a number to the hostname.
    const auto uniqueTag = getMDNSHostname();
    // TODO We should display this tag somewhere, otherwise how do we know how to connect to the device?
    ESP_LOGI(TAG, "Unique tag: %s", uniqueTag.c_str());
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set(uniqueTag.c_str()));
    ESP_ERROR_CHECK(mdns_instance_name_set(uniqueTag.c_str()));

    mdns_txt_item_t serviceTxtData[] = {{"board", "esp32"}, {"path", "/"}};
    ESP_ERROR_CHECK(mdns_service_add("SunTransit Configuration Server", "_http", "_tcp", 80, serviceTxtData,
                                     sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));

    netbiosns_init();
    netbiosns_set_name("suntransit");
}

static void wifi_prov_print_qr(const std::string &name) {
    const std::string payload = "{\"ver\":\"v1\",\"name\":\"" + name + "\",\"transport\":\"softap\"}";
    provisioning_screen.addQRCode(payload);
}

void init_network_wifi_and_wifimanager() {
    /* Initialize TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());

    init_mdns_and_netbios();

    wifi_event_group = xEventGroupCreate();

    /* Register our event handlers for Wi-Fi/IP related events */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &provisioning_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &provisioning_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &provisioning_event_handler, NULL));

    /* Initialize Wi-Fi including netif with default config */
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Configuration for the provisioning manager */
    wifi_prov_mgr_config_t config = {.scheme = wifi_prov_scheme_softap,
                                     .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE};

    /* Initialize provisioning manager with the configuration parameters set above */
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
}

QueueHandle_t departuresRefreshQueue = xQueueCreate(1, sizeof(uint8_t));

void fetch_and_process_trips(BvgApiClient &apiClient) {
    ESP_LOGD(TAG, "Fetching trips...");
    NVSEngine nvs_engine("suntransit");

    JsonDocument settingsDoc;
    auto err = nvs_engine.readSettings(&settingsDoc);
    if (err) {
        ESP_LOGE(TAG, "Failed to read settings from NVS");
        const ui_lock_guard lock;
        departures_screen.showStationNotFoundError();
        return;
    }

    if (settingsDoc["currentStation"].isNull()) {
        ESP_LOGD(TAG, "No current station configured");
        // TODO Do not repeat this all the time, save the status and update the screen only on change
        const ui_lock_guard lock;
        departures_screen.showStationNotFoundError();
        return;
    }

    // TODO When the station is configured initially, the "station not found" message
    // remains until the first reboot. Fix
    auto currentStationDoc = settingsDoc["currentStation"];
    int minDepartureMinutes = settingsDoc["minDepartureMinutes"];
    int maxDepartureCount = settingsDoc["maxDepartureCount"];
    bool showCancelledDepartures = settingsDoc["showCancelledDepartures"];

    ESP_LOGD(TAG, "Minimum departure minutes filter: %d", minDepartureMinutes);
    ESP_LOGD(TAG, "Maximum departure count: %d", maxDepartureCount);
    ESP_LOGD(TAG, "Show cancelled departures: %s", showCancelledDepartures ? "true" : "false");

    auto enabledProductsJsonArray = currentStationDoc["enabledProducts"].as<JsonArrayConst>();
    std::vector<std::string> enabledProducts;
    for (auto enabledProduct : enabledProductsJsonArray) {
        enabledProducts.push_back(enabledProduct.as<std::string>());
    }
    auto trips = apiClient.fetchAndParseTrips(currentStationDoc["id"], enabledProducts, maxDepartureCount);
    ESP_LOGD(TAG, "Fetched and parsed %d trips", trips.size());

    if (trips.empty()) {
        ESP_LOGE(TAG, "No trips found!");
        return;
    }

    {
        // Update departures screen with tripId-based management for efficient updates
        const ui_lock_guard lock;
        const auto now = Time::timePointNow();

        // Keep track of current tripIds to remove stale items
        std::unordered_set<std::string> currentTripIds;

        for (auto trip : trips) {
            // For cancelled trips (when=null), use plannedTime; for active trips, use departureTime
            const bool isCancelled = !trip.departureTime.has_value();
            const auto timeToDisplay = isCancelled ? trip.plannedTime : trip.departureTime.value();

            const auto timeToDeparture = std::chrono::duration_cast<std::chrono::seconds>(timeToDisplay - now);

            if (minDepartureMinutes > 0) {
                const auto minDepartureSeconds = std::chrono::seconds(minDepartureMinutes * 60);
                if (timeToDeparture < minDepartureSeconds) {
                    ESP_LOGD(TAG, "Filtering out trip %s (departure in %ld seconds, minimum is %ld seconds)",
                             trip.tripId.c_str(), timeToDeparture.count(), minDepartureSeconds.count());
                    continue;
                }
            }

            if (!showCancelledDepartures && isCancelled) {
                ESP_LOGD(TAG, "Filtering out cancelled trip %s", trip.tripId.c_str());
                continue;
            }

            currentTripIds.insert(trip.tripId);
            departures_screen.updateDepartureItem(trip.tripId, trip.lineName, trip.directionName, timeToDeparture,
                                                  isCancelled);
        }

        // Remove items that are no longer in the current data
        std::vector<std::string> itemsToRemove;
        for (const auto &[tripId, item] : departures_screen.getDepartureItems()) {
            if (currentTripIds.find(tripId) == currentTripIds.end()) {
                itemsToRemove.push_back(tripId);
            }
        }
        for (const auto &tripId : itemsToRemove) {
            departures_screen.removeDepartureItem(tripId);
        }

        departures_screen.reorderByDepartureTime();

        departures_screen.updateLastUpdatedTime();
    }
    ESP_LOGD(TAG, "Done processing trips");
}

void DeparturesRefresherTask(void *pvParameter) {
    uint8_t message;

    auto apiClient = BvgApiClient();

    while (true) {
        if (xQueueReceive(departuresRefreshQueue, &message, 0) == pdPASS) {
            fetch_and_process_trips(apiClient);
        }
        std::this_thread::sleep_for(10ms);
    }

    vTaskDelete(NULL);
}

const esp_timer_create_args_t departuresRefresherTimerArgs = {
    .callback =
        [](void *arg) {
            uint8_t message = 1;
            xQueueSend(departuresRefreshQueue, &message, 0);
        },
    .name = "departuresRefreshTimer",
};

esp_timer_handle_t departuresRefreshTimerHandle = nullptr;

const esp_timer_create_args_t lastUpdatedRefreshTimerArgs = {
    .callback = [](void *arg) { departures_screen.refreshLastUpdatedDisplay(); },
    .name = "lastUpdatedRefreshTimer",
};

esp_timer_handle_t lastUpdatedRefreshTimerHandle = nullptr;

static void wifi_timeout_callback(void *arg) {
    ESP_LOGW(TAG, "WiFi connection timeout after 20 seconds, showing reset button");
    splash_screen.showConnectingToWiFiWithResetButton(reset_wifi_and_reboot);
}

extern "C" void app_main(void) {
    printHealthStats("app_main start");
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    LVGL_LCD::init();
    UIManager::init();

    NVSEngine::init();
    init_network_wifi_and_wifimanager();

    xTaskCreatePinnedToCore(DeparturesRefresherTask, "DeparturesRefresherTask", 1024 * 5, NULL, 1, NULL, 1);

    bool provisioned = false;
    /* Let's find out if the device is provisioned */
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

    if (!provisioned) {
        ESP_LOGI(TAG, "Starting provisioning");

        std::string service_name = getProvisioningSSID();
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_0, nullptr, service_name.c_str(), nullptr));
        provisioning_screen.switchTo();
        provisioning_screen.showSetupInstructions();
        provisioning_screen.showAppProvisioningInstructions();
        wifi_prov_print_qr(service_name.c_str());
    } else {
        ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi STA");
        splash_screen.showConnectingToWiFi();

        /* We don't need the manager as device is already provisioned, so let's release it's resources */
        wifi_prov_mgr_deinit();

        /* Start Wi-Fi in station mode */
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());
    }

    if (provisioned) {
        /* Start a 20-second timer to show reset button if WiFi doesn't connect */
        const esp_timer_create_args_t wifi_timeout_timer_args = {
            .callback = wifi_timeout_callback,
            .name = "wifi_timeout_timer",
        };
        ESP_ERROR_CHECK(esp_timer_create(&wifi_timeout_timer_args, &wifi_timeout_timer));
        ESP_ERROR_CHECK(
            esp_timer_start_once(wifi_timeout_timer, duration_cast<std::chrono::microseconds>(20s).count()));
    }

    /* Wait for Wi-Fi connection */
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_EVENT, true, true, portMAX_DELAY);

    if (provisioned) {
        splash_screen.showConnectedSwitchingToMain();
    } else {
        provisioning_screen.showWiFiConnectedMessage();
    }

    std::this_thread::sleep_for(2s);

    setup_http_server();

    // TODO We should avoid starting the timer before we have a valid time from NTP
    Time::initSNTP();

    departures_screen.switchTo();
    ESP_ERROR_CHECK(esp_timer_create(&departuresRefresherTimerArgs, &departuresRefreshTimerHandle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(
        departuresRefreshTimerHandle,
        std::chrono::duration_cast<std::chrono::microseconds>(DEPARTURES_REFRESH_PERIOD).count()));

    ESP_ERROR_CHECK(esp_timer_create(&lastUpdatedRefreshTimerArgs, &lastUpdatedRefreshTimerHandle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(
        lastUpdatedRefreshTimerHandle,
        std::chrono::duration_cast<std::chrono::microseconds>(LAST_UPDATED_REFRESH_PERIOD).count()));

    printHealthStats("end of app_main");
};
