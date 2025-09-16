#pragma once

#include "lvgl.h"
#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

// Cross-platform LVGL mutex handling
#ifdef ESP_PLATFORM
#include "esp_lvgl_port.h"

class ui_lock_guard {
  public:
    ui_lock_guard() { lvgl_port_lock(0); }
    ~ui_lock_guard() { lvgl_port_unlock(); }
};

#else
// Simulator build - use std::recursive_mutex
inline std::recursive_mutex lvgl_mutex;

class ui_lock_guard {
  public:
    ui_lock_guard() : lock_(lvgl_mutex) {}

  private:
    std::lock_guard<std::recursive_mutex> lock_;
};
#endif

// TODO Think about using binary fonts to reduce memory usage/space
LV_FONT_DECLARE(montserrat_regular_16);
LV_FONT_DECLARE(roboto_condensed_light_28_4bpp);
LV_FONT_DECLARE(roboto_condensed_regular_28_4bpp);
LV_FONT_DECLARE(montserrat_regular_72);

class Screen {
  public:
    void switchTo(lv_scr_load_anim_t anim_type = LV_SCR_LOAD_ANIM_NONE, uint32_t time = 0, uint32_t delay = 0);

  protected:
    lv_obj_t *createPanel(lv_scroll_snap_t snap_type);
    lv_obj_t *screen = nullptr;

    virtual void init() = 0;
};

class SplashScreen : public Screen {
  public:
    void init();
    void updateStatus(const std::string &message);

    void showInitializingWiFi();
    void showStartingProvisioning();
    void showConnectingToWiFi();
    void showConnectingToWiFiWithResetButton(void (*reset_callback)());
    void showConnectedSwitchingToMain();

  private:
    lv_obj_t *title = nullptr;
    lv_obj_t *status = nullptr;
    lv_obj_t *spinner = nullptr;
    lv_obj_t *reset_button = nullptr;
    void (*reset_callback_fn)() = nullptr;
};

class ProvisioningScreen : public Screen {
  public:
    void init();
    void addLine(const std::string &message);
    void addQRCode(const std::string &data, const int size = 120);

    void showSetupInstructions();
    void showWiFiConnectedMessage();
    void showAppProvisioningInstructions();

  private:
    static const int MAX_LINES = 100;
    lv_obj_t *heading = nullptr;
    lv_obj_t *panel = nullptr;
};

class DepartureItem {
  public:
    void create(lv_obj_t *parent, const std::string &line_text, const std::string &direction_text,
                const std::string &time_text, const std::optional<std::chrono::seconds> &time_to_departure);
    void update(const std::string &line_text, const std::string &direction_text, const std::string &time_text,
                const std::optional<std::chrono::seconds> &time_to_departure);
    void destroy();
    bool isValid() const { return item != nullptr; }
    lv_obj_t *getItem() const { return item; }
    std::optional<std::chrono::seconds> getDepartureTime() const { return departure_time; }

  private:
    lv_obj_t *item = nullptr;
    lv_obj_t *line = nullptr;
    lv_obj_t *direction = nullptr;
    lv_obj_t *time = nullptr;
    std::optional<std::chrono::seconds> departure_time;
};

class DeparturesScreen : public Screen {
  public:
    void init();
    void updateDepartureItem(const std::string &trip_id, const std::string &line_text,
                             const std::string &direction_text,
                             const std::optional<std::chrono::seconds> &time_to_departure);
    void removeDepartureItem(const std::string &trip_id);
    void addTextItem(const std::string &text);
    void clean();
    void cleanDepartureItems();
    void updateLastUpdatedTime();
    void refreshLastUpdatedDisplay();
    void reorderByDepartureTime();
    const std::unordered_map<std::string, DepartureItem> &getDepartureItems() const { return departure_items; }

    void showLoadingMessage(const std::string &station_name);
    void showStationNotFoundError();

  private:
    lv_obj_t *line = nullptr;
    lv_obj_t *direction = nullptr;
    lv_obj_t *departure = nullptr;
    lv_obj_t *panel = nullptr;
    lv_obj_t *last_updated_label = nullptr;
    std::chrono::system_clock::time_point last_updated_time;
    std::unordered_map<std::string, DepartureItem> departure_items;
};

inline SplashScreen splash_screen;
inline ProvisioningScreen provisioning_screen;
inline DeparturesScreen departures_screen;

namespace UIManager {
void init();
}
