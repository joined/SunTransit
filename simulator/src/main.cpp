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

static int timestamp_refresh_thread(void *data) {
    while (true) {
        this_thread::sleep_for(1000ms);
        departures_screen.refreshLastUpdatedDisplay();
    }
    return 0;
}

static int ui_thread(void *data) {
    UIManager::init();

    // TODO We should create wrappers in the screen classes for these operations
    // so that simulator code and embedded code stay in sync.

    // We start from splash screen
    splash_screen.updateStatus("Initializing WiFi...");
    this_thread::sleep_for(1500ms);

    splash_screen.updateStatus("Starting provisioning...");
    this_thread::sleep_for(1000ms);

    // Switch to provisioning screen
    provisioning_screen.switchTo(LV_SCR_LOAD_ANIM_MOVE_LEFT, 300);
    this_thread::sleep_for(500ms);

    provisioning_screen.addLine("Welcome to SunTransit!");
    this_thread::sleep_for(300ms);
    provisioning_screen.addLine("Setting up your Berlin transport display...");
    this_thread::sleep_for(500ms);
    provisioning_screen.addLine("");
    provisioning_screen.addLine("Connect your phone to WiFi network:");
    provisioning_screen.addLine("Network: " + AP_SSID);
    provisioning_screen.addLine("Password: " + AP_PASSWORD);
    this_thread::sleep_for(1000ms);

    provisioning_screen.addLine("");
    provisioning_screen.addLine("Or scan this QR code:");
    provisioning_screen.addQRCode(PROVISIONING_QR_CODE_DATA, 140);
    this_thread::sleep_for(2000ms);

    provisioning_screen.addLine("");
    provisioning_screen.addLine("Then open http://suntransit.local/ in your browser");
    provisioning_screen.addLine("to configure your station and WiFi settings.");
    this_thread::sleep_for(3000ms);

    // Simulate provisioning process
    provisioning_screen.addLine("");
    provisioning_screen.addLine("Waiting for configuration...");
    this_thread::sleep_for(2000ms);

    provisioning_screen.addLine("WiFi configured: MyHomeWiFi");
    this_thread::sleep_for(800ms);
    provisioning_screen.addLine("Station configured: S+U Alexanderplatz");
    this_thread::sleep_for(800ms);
    provisioning_screen.addLine("Settings saved!");
    this_thread::sleep_for(1000ms);

    provisioning_screen.addLine("");
    provisioning_screen.addLine("Loading departures...");
    this_thread::sleep_for(1500ms);

    // Switch to departures screen
    departures_screen.switchTo(LV_SCR_LOAD_ANIM_MOVE_LEFT, 400);
    this_thread::sleep_for(500ms);

    // Show initial loading state
    departures_screen.addTextItem("Loading departures for S+U Alexanderplatz...");
    this_thread::sleep_for(1500ms);

    departures_screen.clean();
    departures_screen.updateLastUpdatedTime();

    // Start timestamp refresh thread
    SDL_CreateThread(timestamp_refresh_thread, "timestamp_refresh", NULL);

    // Add realistic Berlin departures
    uniform_int_distribution<> line_dist(0, BERLIN_LINES.size() - 1);
    uniform_int_distribution<> time_dist(0, 25);

    for (int i = 0; i < 12; i++) {
        const auto &line_data = BERLIN_LINES[line_dist(gen)];
        auto departure_time = chrono::minutes(time_dist(gen));

        // Occasionally add "Now" departures
        if (i % 4 == 0) {
            departure_time = chrono::minutes(0);
        }

        departures_screen.addDepartureItem(line_data.line, line_data.direction, departure_time);
        this_thread::sleep_for(300ms);
    }

    // Simulate periodic updates
    while (true) {
        this_thread::sleep_for(5000ms);

        // Occasionally add new departures to show live updates
        if (gen() % 3 == 0) {
            this_thread::sleep_for(2000ms);
            const auto &line_data = BERLIN_LINES[line_dist(gen)];
            auto departure_time = chrono::minutes(time_dist(gen));
            // TODO Remove oldest item if too many items?
            departures_screen.addDepartureItem(line_data.line, line_data.direction, departure_time);
            departures_screen.updateLastUpdatedTime();
        }
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
