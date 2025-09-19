#include <ArduinoJson.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include <string>

class NVSEngine {
  public:
    NVSEngine(std::string nspace, nvs_open_mode mode = NVS_READWRITE);
    ~NVSEngine();
    static void init();
    esp_err_t readString(const std::string &key, std::string *result);
    esp_err_t setString(const std::string &key, const std::string &value);
    esp_err_t readSettings(JsonDocument *doc);
    esp_err_t setSettings(const JsonDocument &doc);
    esp_err_t initializeDefaultSettingsIfMissing();

  private:
    nvs_handle_t handle;
    static constexpr const char *APP_NVS_PARTITION = "app_nvs";
};
