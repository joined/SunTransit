#include <esp_log.h>
#include <map>
#include <variant>

#include "nvs_engine.hpp"

static const char *TAG = "NVS";

static const std::map<std::string, std::variant<int, bool, std::nullptr_t>> DEFAULT_SETTINGS = {
    {"minDepartureMinutes", 0},
    {"maxDepartureCount", 12},
    {"showCancelledDepartures", true},
    {"currentStation", nullptr}};

// Application data is stored in a separate NVS partition (app_nvs) which can be erased
// independently without affecting WiFi config stored in the default NVS partition
// To erase app data: `parttool.py erase_partition --partition-name=app_nvs`

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

    // Initialize default settings if they don't exist
    NVSEngine nvs_settings("suntransit");
    nvs_settings.initializeDefaultSettingsIfMissing();
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

esp_err_t NVSEngine::readSettings(JsonDocument *doc) {
    std::string settings;
    auto err = this->readString("settings", &settings);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read settings from NVS: %s", esp_err_to_name(err));
        return err;
    }

    auto deserializationError = deserializeJson(*doc, settings);
    if (deserializationError) {
        ESP_LOGE(TAG, "Failed to parse settings JSON: %s", deserializationError.c_str());
        return ESP_FAIL;
    }

    return ESP_OK;
};

esp_err_t NVSEngine::setSettings(const JsonDocument &doc) {
    std::string settings;
    serializeJson(doc, settings);
    return this->setString("settings", settings);
};

esp_err_t NVSEngine::initializeDefaultSettingsIfMissing() {
    std::string settings;
    auto err = this->readString("settings", &settings);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "Settings not found in NVS, initializing with defaults");
        JsonDocument defaultDoc;
        for (const auto &[key, value] : DEFAULT_SETTINGS) {
            std::visit([&](const auto &v) { defaultDoc[key] = v; }, value);
        }
        return this->setSettings(defaultDoc);
    }
    return ESP_OK;
};
