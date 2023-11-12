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

  private:
    nvs_handle_t handle;
};
