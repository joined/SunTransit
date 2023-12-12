#include <chrono>
#include <ctime>
#include <esp_http_client.h>
#include <esp_log.h>
#include <map>
#include <string>
#include <vector>

#include "bvg_api_client.hpp"
#include "nvs_engine.hpp"
#include "time.hpp"

static const char *TAG = "BvgApiClient";

const std::vector<std::string> ALL_PRODUCTS = {"suburban", "subway", "tram", "bus", "ferry", "express", "regional"};

BvgApiClient::BvgApiClient(const std::string &stationId) {
    esp_http_client_config_t config = {
        .url = "https://www.google.com", // Set later
        .user_agent = "DepMon gasparini.lorenzo@gmail.com",
        .timeout_ms = 10000, // Seems to help with timeout issues
        .event_handler =
            [](esp_http_client_event_t *evt) {
                auto self = static_cast<BvgApiClient *>(evt->user_data);
                return self->http_event_handler(evt);
            },
        .user_data = this,
    };
    client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_http_client_set_header(client, "Accept", "application/json");
}

BvgApiClient::~BvgApiClient() { esp_http_client_cleanup(client); }

esp_err_t BvgApiClient::http_event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, data_len=%d", evt->data_len);
        ESP_LOGD(TAG, "Current buffer size: %d", this->current_buffer_size);
        ESP_LOGD(TAG, "Current buffer_pos: %d", buffer_pos);

        if (this->output_buffer == NULL) {
            this->current_buffer_size = HTTP_BUFFER_START_SIZE;
            this->output_buffer = (char *)malloc(this->current_buffer_size);
            this->buffer_pos = 0;
            if (this->output_buffer == NULL) {
                ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                return ESP_FAIL;
            }
        } else {
            while (this->buffer_pos + evt->data_len >= this->current_buffer_size) {
                ESP_LOGD(TAG, "Would overflow buffer, reallocating");
                this->current_buffer_size *= this->BUFFER_GROWTH_FACTOR;
                this->output_buffer = (char *)realloc(this->output_buffer, this->current_buffer_size);

                if (this->output_buffer == NULL) {
                    ESP_LOGE(TAG, "Failed to reallocate memory for output buffer");
                    return ESP_FAIL;
                }
            }
        }
        memcpy(this->output_buffer + buffer_pos, evt->data, evt->data_len);
        this->buffer_pos += evt->data_len;
        break;

    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        this->buffer_pos = 0;
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        free(this->output_buffer);
        this->output_buffer = NULL;
        buffer_pos = 0;
        break;
    default:
        break;
    }

    return ESP_OK;
}

void BvgApiClient::setUrl(const std::string &stationId, const std::vector<std::string> &enabledProducts) {
    // TODO store and read number of results from NVS. keep in mind that memory is limited.
    std::map<std::string, std::string> queryParams = {
        {"results", std::to_string(N_RESULTS)},
        {"pretty", "false"},
        // TODO Extract to constant
        {"duration", std::to_string(60)},
    };
    for (const auto &product : ALL_PRODUCTS) {
        queryParams[product] = "false";
    }
    for (const auto &product : enabledProducts) {
        queryParams[product] = "true";
    }

    std::ostringstream url;

    url << "https://v6.bvg.transport.rest/stops/" << stationId << "/departures?";

    for (auto entry = queryParams.begin(); entry != queryParams.end(); ++entry) {
        if (entry != queryParams.begin()) {
            url << "&";
        }

        // TODO URLEncode / escape? We don't really need it
        url << entry->first << "=" << entry->second;
    }

    esp_http_client_set_url(client, url.str().c_str());
}

std::vector<Trip> BvgApiClient::fetchAndParseTrips(const std::string &stationId,
                                                   const std::vector<std::string> &enabledProducts) {
    this->setUrl(stationId, enabledProducts);
    auto err = esp_http_client_perform(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        return {};
    }

    if (this->output_buffer == NULL) {
        ESP_LOGE(TAG, "No response buffer, something went wrong");
        return {};
    }

    JsonDocument filter;
    filter["departures"][0]["tripId"] = true;
    filter["departures"][0]["direction"] = true;
    filter["departures"][0]["line"]["name"] = true;
    filter["departures"][0]["when"] = true;

    JsonDocument doc;
    // TODO It would be cool to use a std::istream here, would probably save memory too.
    auto deserializationError = deserializeJson(doc, this->output_buffer, DeserializationOption::Filter(filter));
    if (deserializationError) {
        ESP_LOGE(TAG, "Failed to parse JSON: %s", deserializationError.c_str());
        return {};
    }

    JsonArray departures = doc["departures"];
    auto departure_count = departures.size();
    ESP_LOGD(TAG, "Got %d departures", departure_count);

    std::vector<Trip> trips;
    trips.reserve(departure_count);

    Trip trip;
    for (auto departure : departures) {
        const char *tripId = departure["tripId"];
        const char *direction = departure["direction"];
        const char *line = departure["line"]["name"];

        const auto departure_time =
            departure["when"].isNull()
                ? std::nullopt
                : std::make_optional(Time::iSO8601StringToTimePoint(static_cast<const char *>(departure["when"])));

        trips.push_back(
            {.tripId = tripId, .departureTime = departure_time, .directionName = direction, .lineName = line});
    }

    free(this->output_buffer);
    this->output_buffer = NULL;

    return trips;
}
