#pragma once

#include "lvgl.h"
#include <chrono>
#include <mutex>
#include <optional>
#include <string>

inline std::recursive_mutex lvgl_mutex;

/* Used for the small text in the departures panel */
LV_FONT_DECLARE(montserrat_regular_16);
/* Used for the line and the ETD in the departures panel */
LV_FONT_DECLARE(roboto_condensed_light_28_4bpp);
/* Used for the direction in the departures panel. Special features: has `⟲⟳` symbols (U+27F2 and U+27F3, for Ringbahn).
 */
LV_FONT_DECLARE(roboto_condensed_regular_28_4bpp);
/* Used for the big title in the splash screen. Has only the "SunTransit" characters. */
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
    void showConnectedSwitchingToMain();

  private:
    lv_obj_t *title = nullptr;
    lv_obj_t *status = nullptr;
    lv_obj_t *spinner = nullptr;
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

class DeparturesScreen : public Screen {
  public:
    void init();
    void addDepartureItem(const std::string &line_text, const std::string &direction_text,
                          const std::optional<std::chrono::seconds> &time_to_departure);
    void addTextItem(const std::string &text);
    void clean();
    void updateLastUpdatedTime();
    void refreshLastUpdatedDisplay();

    void showLoadingMessage(const std::string &station_name);
    void showStationNotFoundError();

  private:
    lv_obj_t *line = nullptr;
    lv_obj_t *direction = nullptr;
    lv_obj_t *departure = nullptr;
    lv_obj_t *panel = nullptr;
    lv_obj_t *last_updated_label = nullptr;
    std::chrono::system_clock::time_point last_updated_time;
};

inline SplashScreen splash_screen;
inline ProvisioningScreen provisioning_screen;
inline DeparturesScreen departures_screen;

namespace UIManager {
void init();
}
