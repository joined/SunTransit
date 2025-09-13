#include <ArduinoJson.h>
#include <esp_app_desc.h>
#include <esp_chip_info.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_random.h>
#include <esp_spiffs.h>
#include <esp_system.h>
#include <esp_vfs.h>
#include <fcntl.h>
#include <ranges>
#include <string>
#include <vector>

#include "http_server.hpp"
#include "nvs_engine.hpp"
#include "time.hpp"
#include "utils.hpp"

static const constexpr size_t FS_READ_BUFFER_SIZE = 30 * 1024;
// TODO It's a bit of a waste to have this static buffer, can we do better?
// Ideally we'd be using memory only when the request is being handled.
// -> should probably increase stack size of the httpd task to make it a local buffer
static char scratch[FS_READ_BUFFER_SIZE];
static const char *TAG = "http_server";

static esp_err_t init_fs(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/www", .partition_label = NULL, .max_files = 5, .format_if_mount_failed = false};
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGD(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    return ESP_OK;
}

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const std::string &filepath) {
    const std::string cleaned_filepath =
        filepath.ends_with(".gz") ? filepath.substr(0, filepath.length() - 3) : filepath;
    const char *type = "text/plain";
    if (cleaned_filepath.ends_with(".html")) {
        type = "text/html";
    } else if (cleaned_filepath.ends_with(".js")) {
        type = "application/javascript";
    } else if (cleaned_filepath.ends_with(".css")) {
        type = "text/css";
    } else if (cleaned_filepath.ends_with(".png")) {
        type = "image/png";
    } else if (cleaned_filepath.ends_with(".ico")) {
        type = "image/x-icon";
    } else if (cleaned_filepath.ends_with(".svg")) {
        type = "text/xml";
    }
    return httpd_resp_set_type(req, type);
}

const std::vector<std::string> INDEX_ROUTES = {"/", "/sysinfo"};

/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t *req) {
    // TODO Make this more dynamic
    std::string filepath = "/www";
    const std::string uri = req->uri;
    const auto is_index_route = std::ranges::find(INDEX_ROUTES, uri) != INDEX_ROUTES.end();
    if (is_index_route) {
        filepath += "/index.html";
    } else {
        filepath += uri;
    }
    // Assumption: all files are gzipped. This won't be the case forever, probably.
    filepath += ".gz";

    ESP_LOGD(TAG, "Opening file %s", filepath.c_str());

    int file_descriptor = open(filepath.c_str(), O_RDONLY, 0);
    if (file_descriptor == -1) {
        ESP_LOGE(TAG, "Failed to open file : %s", filepath.c_str());
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");

    if (!is_index_route) {
        // Assets like CSS and JS have a cache busting hash in their filename.
        httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=604800, immutable");
    }

    ESP_LOGD(TAG, "Starting to send response for file: %s", filepath.c_str());

    char *chunk = scratch;
    ssize_t read_bytes;
    do {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(file_descriptor, chunk, FS_READ_BUFFER_SIZE);
        if (read_bytes == -1) {
            ESP_LOGE(TAG, "Failed to read file : %s", filepath.c_str());
        } else if (read_bytes > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
                close(file_descriptor);
                ESP_LOGE(TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    /* Close file after sending complete */
    close(file_descriptor);
    ESP_LOGD(TAG, "File sending complete for file: %s", filepath.c_str());
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t api_get_sysinfo_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");

    JsonDocument doc;
    auto software = doc["software"].to<JsonObject>();
    const esp_app_desc_t *app_description = esp_app_get_description();
    software["app_version"] = app_description->version;
    software["idf_version"] = app_description->idf_ver;
    software["project_name"] = app_description->project_name;
    software["compile_time"] = app_description->time;
    software["compile_date"] = app_description->date;

    auto app_state = doc["app_state"].to<JsonObject>();
    // TODO What if time is not configured?
    auto time = Time::epochMillis();
    app_state["time"] = time;
    app_state["mdns_hostname"] = getMDNSHostname() + ".local";

    auto hardware = doc["hardware"].to<JsonObject>();
    hardware["mac_address"] = getMacString(true);
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    hardware["chip_model"] = chip_info.model;

    auto memory = doc["memory"].to<JsonObject>();
    memory["free_heap"] = esp_get_free_heap_size();
    memory["minimum_free_heap"] = esp_get_minimum_free_heap_size();
    // TODO The following line seems to be causing panics. Investigate.
    // memory["largest_free_heap_block"] = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);

    // TODO Add total runtime?

#ifdef CONFIG_FREERTOS_USE_TRACE_FACILITY
    const auto task_count = uxTaskGetNumberOfTasks();
    TaskStatus_t *pxTaskStatusArray = (TaskStatus_t *)malloc(sizeof(TaskStatus_t) * task_count);
    if (pxTaskStatusArray == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate memory for task status array");
    } else {
        auto tasks = doc["tasks"].to<JsonArray>();
#ifdef CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
        unsigned long ulTotalRunTime;
        uxTaskGetSystemState(pxTaskStatusArray, task_count, &ulTotalRunTime);
        ulTotalRunTime /= 100UL;
#else
        uxTaskGetSystemState(pxTaskStatusArray, task_count, nullptr);
#endif
        for (auto i = 0; i < task_count; i++) {
            TaskStatus_t task = pxTaskStatusArray[i];
            auto task_json = tasks.add<JsonObject>();
            task_json["name"] = task.pcTaskName;
            task_json["priority"] = task.uxCurrentPriority;
            task_json["state"] = task.eCurrentState;
            task_json["stack_high_water_mark"] = task.usStackHighWaterMark;
#ifdef CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
            int core_id = task.xCoreID;
            task_json["core_id"] = core_id == INT32_MAX ? -1 : core_id;
#else
            task_json["core_id"] = nullptr;
#endif
#ifdef CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
            task_json["runtime"] = task.ulRunTimeCounter / ulTotalRunTime;
#else
            task_json["runtime"] = nullptr;
#endif
        }
        free(pxTaskStatusArray);
    }
#else
    doc["tasks"] = nullptr;
#endif

    char buffer[2048];
    const auto bytesWritten = serializeJson(doc, buffer);
    if (bytesWritten == 0) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to serialize JSON");
        return ESP_FAIL;
    }
    httpd_resp_send(req, buffer, bytesWritten);
    return ESP_OK;
}

static esp_err_t api_get_settings_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");

    NVSEngine nvs_engine("suntransit");
    JsonDocument settings_doc;
    auto err = nvs_engine.readSettings(&settings_doc);
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read settings");
        return ESP_FAIL;
    }

    char buffer[512];
    const auto bytesWritten = serializeJson(settings_doc, buffer);
    if (bytesWritten == 0) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to serialize settings JSON");
        return ESP_FAIL;
    }
    httpd_resp_send(req, buffer, bytesWritten);
    return ESP_OK;
}

static esp_err_t api_set_settings_handler(httpd_req_t *req) {
    char content[2048];
    size_t recv_size = std::min(req->content_len, sizeof(content));
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) { /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            /* In case of timeout one can choose to retry calling
             * httpd_req_recv(), but to keep it simple, here we
             * respond with an HTTP 408 (Request Timeout) error */
            httpd_resp_send_408(req);
        }
        /* In case of error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
    }

    JsonDocument settings_doc;
    auto deserializationError = deserializeJson(settings_doc, content, recv_size);
    if (deserializationError) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    NVSEngine nvs_engine("suntransit");
    JsonDocument currentSettings;
    auto readErr = nvs_engine.readSettings(&currentSettings);
    if (readErr != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read current settings");
        return ESP_FAIL;
    }

    if (settings_doc["minDepartureMinutes"].is<JsonVariant>()) {
        if (!settings_doc["minDepartureMinutes"].is<int>()) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "minDepartureMinutes must be a number");
            return ESP_FAIL;
        }
        int minDepartureMinutes = settings_doc["minDepartureMinutes"];
        if (minDepartureMinutes < 0 || minDepartureMinutes > 30) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "minDepartureMinutes must be between 0 and 30");
            return ESP_FAIL;
        }
        currentSettings["minDepartureMinutes"] = minDepartureMinutes;
    }

    // Validate and update currentStation if provided
    if (settings_doc["currentStation"].is<JsonVariant>()) {
        auto currentStation = settings_doc["currentStation"];
        if (!currentStation["id"].is<JsonVariant>() || !currentStation["id"].is<const char *>()) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "currentStation.id is required and must be a string");
            return ESP_FAIL;
        }
        if (!currentStation["enabledProducts"].is<JsonArrayConst>()) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                                "currentStation.enabledProducts is required and must be an array");
            return ESP_FAIL;
        }
        currentSettings["currentStation"] = currentStation;
    }

    auto err = nvs_engine.setSettings(currentSettings);
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save settings");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{}");
    return ESP_OK;
}

httpd_handle_t setup_http_server() {
    init_fs();
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 1024 * 5;
    config.uri_match_fn = httpd_uri_match_wildcard;
    httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(httpd_start(&server, &config));

    httpd_uri_t api_get_sysinfo_uri = {
        .uri = "/api/sysinfo",
        .method = HTTP_GET,
        .handler = api_get_sysinfo_handler,
    };
    httpd_register_uri_handler(server, &api_get_sysinfo_uri);

    httpd_uri_t api_get_settings_uri = {
        .uri = "/api/settings",
        .method = HTTP_GET,
        .handler = api_get_settings_handler,
    };
    httpd_register_uri_handler(server, &api_get_settings_uri);

    httpd_uri_t api_set_settings_uri = {
        .uri = "/api/settings",
        .method = HTTP_POST,
        .handler = api_set_settings_handler,
    };
    httpd_register_uri_handler(server, &api_set_settings_uri);

    /* URI handler for getting web server files */
    httpd_uri_t common_get_uri = {.uri = "/*", .method = HTTP_GET, .handler = rest_common_get_handler};
    httpd_register_uri_handler(server, &common_get_uri);

    return server;
}
