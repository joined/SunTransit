#include "lvgl_sdl.h"
#include <chrono>
#include <string>
#include <thread>

using namespace std::chrono_literals;
using namespace std;

static const string AP_SSID = "esp32";
static const string AP_PASSWORD = "whatever";
static const string PROVISIONING_QR_CODE_DATA = "WIFI:S:" + AP_SSID + ";T:WPA;P:" + AP_PASSWORD + ";;";

static int ui_thread(void *data) {
    UIManager::init();

    this_thread::sleep_for(2s);

    provisioning_screen.switchTo();
    provisioning_screen.addLine("It looks like you're trying to set up your device.");
    provisioning_screen.addLine(
        "Please download the \"ESP SoftAP Provisioning\" app from the App Store or Google Play, "
        "open it and follow the instructions.");
    provisioning_screen.addQRCode("LoremIpsumDolorSitAmet");
    this_thread::sleep_for(2s);

    provisioning_screen.addLine("Connected to WiFi! Switching to departures board...");

    departures_screen.switchTo();
    departures_screen.addTextItem("Station not found.");
    departures_screen.addTextItem("Please access http://suntransit.local/ to configure your station.");

    this_thread::sleep_for(2s);

    departures_screen.clean();
    for (int i = 0; i < 20; i++) {
        departures_screen.addRandomDepartureItem();
        this_thread::sleep_for(200ms);
    }

    return 0;
}

int main(void) {
    LVGL_SDL::init();

    SDL_CreateThread(ui_thread, "ui", NULL);

    // TODO We should probably call `SDL_PollEvent` here and quit on quit event but that seems
    // to mess with LVGL somehow (mouse drag behaves weird).
    // Apparently that might even be required on some OSes.
    while (true) {
        {
            const std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
            // NOTE: `lv_timer_handler` must be called from the same thread that initialized LVGL/SDL
            lv_timer_handler();
        }

        this_thread::sleep_for(5ms);
    }
}
