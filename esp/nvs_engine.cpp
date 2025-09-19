#include <esp_log.h>

#include "nvs_engine.hpp"

static const char *TAG = "NVS";

static constexpr int DEFAULT_MIN_DEPARTURE_MINUTES = 0;
static constexpr int DEFAULT_MAX_DEPARTURE_COUNT = 12;

// Application data is stored in a separate NVS partition (app_nvs) which can be erased
// independently without affecting WiFi config stored in the default NVS partition
// To erase app data: parttool.py erase_partition --partition-name=app_nvs

NVSEngine::NVSEngine(const std::string nspace, nvs_open_mode mode) {
    nvs_open_from_partition(APP_NVS_PARTITION, nspace.c_str(), mode, &this->handle);
};

NVSEngine::~NVSEngine() { nvs_close(this->handle); };

void NVSEngine::init() {
    esp_err_t err = nvs_flash_init();
    ESP_ERROR_CHECK(err);

    err = nvs_flash_init_partition(APP_NVS_PARTITION);
    ESP_ERROR_CHECK(err);

    ESP_LOGD(TAG, "Initialized default and app NVS partitions");
};

esp_err_t NVSEngine::readString(const std::string &key, std::string *result) {
    size_t len;
    auto err = nvs_get_str(this->handle, key.c_str(), nullptr, &len);
    if (err != ESP_OK) {
        return err;
    }

    char *data = (char *)malloc(len);
    nvs_get_str(this->handle, key.c_str(), data, &len);

    *result = std::string(data);
    free(data);

    return ESP_OK;
};

esp_err_t NVSEngine::setString(const std::string &key, const std::string &value) {
    auto err = nvs_set_str(this->handle, key.c_str(), value.c_str());
    if (err) {
        return err;
    }
    err = nvs_commit(this->handle);
    return err;
};

static void initializeDefaultSettings(JsonDocument *doc) {
    (*doc)["minDepartureMinutes"] = DEFAULT_MIN_DEPARTURE_MINUTES;
    (*doc)["maxDepartureCount"] = DEFAULT_MAX_DEPARTURE_COUNT;
    (*doc)["showCancelledDepartures"] = true;
    (*doc)["currentStation"] = nullptr;
}

esp_err_t NVSEngine::readSettings(JsonDocument *doc) {
    std::string settings;
    auto err = this->readString("settings", &settings);
    if (err != ESP_OK) {
        // Initialize default settings
        initializeDefaultSettings(doc);
    } else {
        auto deserializationError = deserializeJson(*doc, settings);
        if (deserializationError) {
            ESP_LOGE(TAG, "Failed to parse settings JSON: %s", deserializationError.c_str());
            // Return default settings if parsing fails
            initializeDefaultSettings(doc);
            return ESP_OK;
        }
    }

    return ESP_OK;
};

esp_err_t NVSEngine::setSettings(const JsonDocument &doc) {
    std::string settings;
    serializeJson(doc, settings);
    return this->setString("settings", settings);
};
