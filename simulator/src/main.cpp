#include "lvgl_sdl.h"
#include <chrono>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;
using namespace std;

static const string AP_SSID = "SunTransit_Setup";
static const string AP_PASSWORD = "suntransit2024";
static const string PROVISIONING_QR_CODE_DATA = "WIFI:S:" + AP_SSID + ";T:WPA;P:" + AP_PASSWORD + ";;";

// Set this to true to simulate a fresh device (not provisioned)
// Set this to false to simulate an already provisioned device
static const bool SIMULATE_NOT_PROVISIONED = false;

// Set this to true to simulate WiFi connection timeout (shows reset button)
static const bool SIMULATE_WIFI_TIMEOUT = false;

// Realistic Berlin transport data
struct BerlinLine {
    string line;
    string direction;
};

static const vector<BerlinLine> BERLIN_LINES = {{"U1", "U Warschauer Str."},
                                                {"U1", "U Uhlandstr."},
                                                {"U2", "U Pankow"},
                                                {"U2", "U Ruhleben"},
                                                {"U3", "U Warschauer Str."},
                                                {"U3", "U Nollendorfplatz"},
                                                {"U4", "U Nollendorfplatz"},
                                                {"U4", "U Innsbrucker Platz"},
                                                {"U5", "U Hönow"},
                                                {"U5", "U Alexanderplatz"},
                                                {"U6", "U Alt-Tegel"},
                                                {"U6", "U Alt-Mariendorf"},
                                                {"U7", "U Rathaus Spandau"},
                                                {"U7", "U Rudow"},
                                                {"U8", "U Wittenau"},
                                                {"U8", "U Hermannstr."},
                                                {"U9", "U Osloer Str."},
                                                {"U9", "S+U Rathaus Steglitz"},
                                                {"S1", "S Wannsee"},
                                                {"S1", "S Oranienburg"},
                                                {"S2", "S Blankenfelde"},
                                                {"S2", "S Bernau"},
                                                {"S3", "S Erkner"},
                                                {"S3", "S Spandau"},
                                                {"S5", "S Mahlsdorf"},
                                                {"S5", "S Westkreuz"},
                                                {"S7", "S Ahrensfelde"},
                                                {"S7", "S Potsdam Hbf"},
                                                {"S8", "S Zeuthen"},
                                                {"S8", "S Birkenwerder"},
                                                {"S9", "S Schönefeld Flughafen"},
                                                {"S9", "S Spandau"},
                                                {"M1", "S Hackescher Markt ⟲"},
                                                {"M1", "Am Kupfergraben ⟳"},
                                                {"M4", "S Hackescher Markt"},
                                                {"M4", "Falkenberg"},
                                                {"M5", "S Hackescher Markt ⟲"},
                                                {"M5", "Zingster Str. ⟳"},
                                                {"M6", "S Hackescher Markt ⟲"},
                                                {"M6", "Riesaer Str. ⟳"},
                                                {"M8", "S Hackescher Markt ⟲"},
                                                {"M8", "Ahrensfelde/Stadtgrenze ⟳"},
                                                {"M10", "S+U Warschauer Str."},
                                                {"M10", "S Nordbahnhof"},
                                                {"142", "S Ostbahnhof"},
                                                {"142", "Ostendstr."},
                                                {"200", "S+U Zoologischer Garten"},
                                                {"200", "Prenzlauer Berg, Michelangelostr."}};

static random_device rd;
static mt19937 gen(rd());

static void mock_reset_wifi_and_reboot() {
    printf("=== SIMULATOR: WiFi reset button clicked! ===\n");
    printf("In real ESP32 this would:\n");
    printf("1. Reset WiFi provisioning\n");
    printf("2. Reboot the device\n");
    printf("3. Start fresh provisioning process\n");
    printf("=== SIMULATOR: Exiting to simulate reboot ===\n");
    exit(0);
}

static int timestamp_refresh_thread(void *data) {
    while (true) {
        this_thread::sleep_for(500ms);
        departures_screen.refreshLastUpdatedDisplay();
    }
    return 0;
}

static int ui_thread(void *data) {
    UIManager::init();

    this_thread::sleep_for(2s);

    // Simulate ESP32 flow based on provisioning state

    if (SIMULATE_NOT_PROVISIONED) {
        provisioning_screen.switchTo(LV_SCR_LOAD_ANIM_MOVE_LEFT, 300);
        this_thread::sleep_for(500ms);

        provisioning_screen.showSetupInstructions();
        provisioning_screen.showAppProvisioningInstructions();
        provisioning_screen.addQRCode(PROVISIONING_QR_CODE_DATA);
        this_thread::sleep_for(3s);

        provisioning_screen.showWiFiConnectedMessage();
        this_thread::sleep_for(2s);
    } else if (SIMULATE_WIFI_TIMEOUT) {
        splash_screen.showConnectingToWiFi();
        printf("SIMULATOR: Simulating 20 second WiFi timeout...\n");
        printf("SIMULATOR: Waiting 3 seconds then showing reset button (compressed time)\n");
        this_thread::sleep_for(3s);

        printf("SIMULATOR: WiFi timeout! Showing reset button\n");
        splash_screen.showConnectingToWiFiWithResetButton(mock_reset_wifi_and_reboot);

        printf("SIMULATOR: Click the reset button to test the reset flow\n");
        printf("SIMULATOR: The program will exit when you click the button\n");

        // Stay in this state indefinitely until user clicks reset button
        while (true) {
            this_thread::sleep_for(100ms);
        }
    } else {
        splash_screen.showConnectingToWiFi();
        this_thread::sleep_for(1.5s);

        splash_screen.showConnectedSwitchingToMain();
        this_thread::sleep_for(2s);
    }

    // Switch to departures screen
    departures_screen.switchTo(LV_SCR_LOAD_ANIM_MOVE_LEFT, 400);
    this_thread::sleep_for(500ms);

    // Demonstrate the station not found error briefly
    departures_screen.showStationNotFoundError();
    this_thread::sleep_for(2s);

    departures_screen.clean();

    // Then show loading again and proceed normally
    departures_screen.showLoadingMessage("S+U Alexanderplatz");
    this_thread::sleep_for(500ms);

    departures_screen.clean();

    // Start timestamp refresh thread
    SDL_CreateThread(timestamp_refresh_thread, "timestamp_refresh", NULL);

    // Generate batch of departures like ESP32 does
    uniform_int_distribution<> line_dist(0, BERLIN_LINES.size() - 1);
    uniform_int_distribution<> time_dist(0, 25);
    uniform_int_distribution<> count_dist(8, 15);

    auto generateAndUpdateDepartures = [&]() {
        int count = count_dist(gen);
        for (int i = 0; i < count; i++) {
            const auto &line_data = BERLIN_LINES[line_dist(gen)];
            auto departure_time = chrono::minutes(time_dist(gen));

            // Occasionally add "Now" departures
            if (i % 4 == 0) {
                departure_time = chrono::minutes(0);
            }

            string trip_id = "sim_trip_" + to_string(i);
            departures_screen.updateDepartureItem(trip_id, line_data.line, line_data.direction, departure_time);
        }
    };

    // Simulate periodic API refreshes (like ESP32 does every 5 seconds)
    while (true) {
        // Generate new batch of departures (simulating new API response)
        generateAndUpdateDepartures();
        departures_screen.reorderByDepartureTime();
        departures_screen.updateLastUpdatedTime();

        this_thread::sleep_for(5s);
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
