#include "ui.hpp"
#include <algorithm>
#include <vector>

namespace Color {
const lv_color_t black = lv_color_hex(0x000000);
const lv_color_t white = lv_color_hex(0xFFFFFF);
const lv_color_t yellow = lv_color_hex(0xFFFF00);
} // namespace Color

static lv_style_selector_t DEFAULT_SELECTOR = (uint32_t)LV_PART_MAIN | (uint16_t)LV_STATE_DEFAULT;

void Screen::switchTo(lv_scr_load_anim_t anim_type, uint32_t time, uint32_t delay) {
    if (screen == nullptr) {
        this->init();
    }

    {
        const ui_lock_guard lock;
        lv_scr_load_anim(screen, anim_type, time, delay, true);
    }
}

lv_obj_t *Screen::createPanel() {
    const ui_lock_guard lock;
    auto *panel = lv_obj_create(screen);
    lv_obj_set_width(panel, lv_pct(100));
    lv_obj_set_height(panel, 250);
    lv_obj_set_y(panel, 5);
    lv_obj_set_align(panel, LV_ALIGN_CENTER);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_radius(panel, 0, DEFAULT_SELECTOR);
    lv_obj_set_style_border_width(panel, 0, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_hor(panel, 10, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_ver(panel, 5, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_row(panel, 2, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_column(panel, 0, DEFAULT_SELECTOR);
    return panel;
};

void SplashScreen::init() {
    const ui_lock_guard lock;
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, Color::black, DEFAULT_SELECTOR);

    title = lv_label_create(screen);
    lv_obj_set_y(title, -40);
    lv_obj_set_align(title, LV_ALIGN_CENTER);
    lv_label_set_text(title, "SunTransit");
    lv_obj_set_style_text_color(title, Color::white, DEFAULT_SELECTOR);
    lv_obj_set_style_text_font(title, &montserrat_regular_72, DEFAULT_SELECTOR);

    status = lv_label_create(screen);
    lv_obj_set_y(status, -30);
    lv_obj_set_align(status, LV_ALIGN_BOTTOM_MID);
    lv_label_set_text(status, "Booting up...");
    lv_obj_set_style_text_color(status, Color::white, DEFAULT_SELECTOR);
    lv_obj_set_style_text_font(status, &montserrat_regular_16, DEFAULT_SELECTOR);

    spinner = lv_spinner_create(screen);
    lv_obj_set_width(spinner, 50);
    lv_obj_set_height(spinner, 51);
    lv_obj_set_y(spinner, 60);
    lv_obj_set_align(spinner, LV_ALIGN_CENTER);

    lv_obj_set_style_arc_color(spinner, Color::yellow, (uint32_t)LV_PART_INDICATOR | (uint16_t)LV_STATE_DEFAULT);
};

void SplashScreen::updateStatus(const std::string &message) {
    if (status == nullptr) {
        return;
    }

    const ui_lock_guard lock;
    lv_label_set_text(status, message.c_str());
};

void ProvisioningScreen::init() {
    const ui_lock_guard lock;
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, Color::black, DEFAULT_SELECTOR);
    lv_obj_set_style_bg_opa(screen, 255, DEFAULT_SELECTOR);

    panel = createPanel();
    lv_obj_set_y(panel, 0);
    lv_obj_set_style_height(panel, lv_pct(85), DEFAULT_SELECTOR);
    lv_obj_set_style_bg_opa(panel, LV_OPA_0, DEFAULT_SELECTOR);
    lv_obj_set_style_text_align(panel, LV_TEXT_ALIGN_CENTER, DEFAULT_SELECTOR);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(panel, 10, DEFAULT_SELECTOR);
};

void ProvisioningScreen::addLine(const std::string &message) {
    if (panel == nullptr) {
        return;
    }

    const ui_lock_guard lock;
    auto *log_line = lv_label_create(panel);
    lv_obj_set_width(log_line, lv_pct(100));
    lv_obj_set_height(log_line, LV_SIZE_CONTENT);
    lv_obj_set_align(log_line, LV_ALIGN_TOP_LEFT);
    lv_label_set_text(log_line, message.c_str());
    lv_obj_set_style_text_font(log_line, &montserrat_regular_16, DEFAULT_SELECTOR);
    lv_obj_set_style_text_color(log_line, Color::white, DEFAULT_SELECTOR);

    lv_obj_scroll_to_y(panel, LV_COORD_MAX, LV_ANIM_OFF);

    if (lv_obj_get_child_cnt(screen) > MAX_LINES) {
        lv_obj_del(lv_obj_get_child(screen, 0));
    }
};

void ProvisioningScreen::addQRCode(const std::string &data, const int size) {
    if (panel == nullptr) {
        return;
    }

    const ui_lock_guard lock;
    auto *qrcode = lv_qrcode_create(panel);
    lv_qrcode_set_size(qrcode, size);
    lv_qrcode_set_dark_color(qrcode, Color::black);
    lv_qrcode_set_light_color(qrcode, Color::white);
    lv_qrcode_update(qrcode, data.c_str(), data.length());

    lv_obj_scroll_to_y(panel, LV_COORD_MAX, LV_ANIM_OFF);
};

void DepartureItem::create(lv_obj_t *parent, const std::string &line_text, const std::string &direction_text,
                           const std::string &time_text, const std::chrono::seconds &time_to_departure,
                           bool is_cancelled) {
    const ui_lock_guard lock;
    departure_time = time_to_departure;

    item = lv_obj_create(parent);
    lv_obj_set_width(item, lv_pct(100));
    lv_obj_set_height(item, LV_SIZE_CONTENT);
    lv_obj_set_align(item, LV_ALIGN_TOP_LEFT);
    lv_obj_set_style_bg_opa(item, 0, DEFAULT_SELECTOR);
    lv_obj_set_style_border_width(item, 0, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_hor(item, 0, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_ver(item, 0, DEFAULT_SELECTOR);
    lv_obj_set_style_text_color(item, Color::yellow, DEFAULT_SELECTOR);
    lv_obj_set_style_text_font(item, &roboto_condensed_regular_28_4bpp, DEFAULT_SELECTOR);
    lv_obj_set_style_text_decor(item, is_cancelled ? LV_TEXT_DECOR_STRIKETHROUGH : LV_TEXT_DECOR_NONE,
                                DEFAULT_SELECTOR);

    line = lv_label_create(item);
    lv_obj_set_align(line, LV_ALIGN_LEFT_MID);
    lv_label_set_text(line, line_text.c_str());

    direction = lv_label_create(item);
    lv_obj_set_height(direction,
                      33); // ideally we'd specify "one line" here but we can't, the value is guessed visually
    lv_obj_set_width(direction, 325);
    lv_obj_set_x(direction, 65);
    lv_obj_set_align(direction, LV_ALIGN_LEFT_MID);
    // Only use the circular mode if the label is really long, otherwise it looks weird.
    // TODO The `30` value is a guess (depends on the characters), we should do better.
    // Maybe we can use `lv_txt_get_size`.
    lv_label_set_long_mode(direction, direction_text.length() > 30 ? LV_LABEL_LONG_SCROLL : LV_LABEL_LONG_DOT);
    lv_label_set_text(direction, direction_text.c_str());
    lv_obj_set_style_text_font(direction, &roboto_condensed_light_28_4bpp, DEFAULT_SELECTOR);

    time = lv_label_create(item);
    lv_obj_set_align(time, LV_ALIGN_RIGHT_MID);
    lv_obj_set_x(time, -8);
    lv_label_set_text(time, time_text.c_str());
}

void DepartureItem::update(const std::string &line_text, const std::string &direction_text,
                           const std::string &time_text, const std::chrono::seconds &time_to_departure,
                           bool is_cancelled) {
    if (item == nullptr) {
        return;
    }

    const ui_lock_guard lock;
    lv_obj_set_style_text_decor(item, is_cancelled ? LV_TEXT_DECOR_STRIKETHROUGH : LV_TEXT_DECOR_NONE,
                                DEFAULT_SELECTOR);
    departure_time = time_to_departure;
    lv_label_set_text(line, line_text.c_str());
    lv_label_set_text(direction, direction_text.c_str());
    lv_label_set_long_mode(direction, direction_text.length() > 30 ? LV_LABEL_LONG_SCROLL : LV_LABEL_LONG_DOT);
    lv_label_set_text(time, time_text.c_str());
}

void DepartureItem::destroy() {
    if (item == nullptr) {
        return;
    }

    const ui_lock_guard lock;
    lv_obj_del(item);
    item = nullptr;
    line = nullptr;
    direction = nullptr;
    time = nullptr;
}

void DeparturesScreen::init() {
    const ui_lock_guard lock;
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, Color::black, DEFAULT_SELECTOR);

    line = lv_label_create(screen);
    lv_obj_set_x(line, 9);
    lv_obj_set_y(line, 7);
    lv_label_set_text(line, "Line");
    lv_obj_set_style_text_font(line, &roboto_condensed_light_28_4bpp, DEFAULT_SELECTOR);
    lv_obj_set_style_text_color(line, Color::white, DEFAULT_SELECTOR);

    direction = lv_label_create(line);
    lv_obj_set_x(direction, 66);
    lv_label_set_text(direction, "Direction");

    departure = lv_label_create(line);
    lv_obj_set_x(departure, 410);
    lv_label_set_text(departure, "ETD");

    panel = createPanel();
    lv_obj_set_style_bg_color(panel, Color::yellow, DEFAULT_SELECTOR);
    lv_obj_set_style_bg_opa(panel, 51, DEFAULT_SELECTOR);

    // Add last updated label at the bottom
    last_updated_label = lv_label_create(screen);
    lv_obj_set_x(last_updated_label, 9);
    lv_obj_set_y(last_updated_label, 295); // Near bottom of 320px screen
    lv_label_set_text(last_updated_label, "Last updated: --");
    lv_obj_set_style_text_color(last_updated_label, Color::white, DEFAULT_SELECTOR);
    lv_obj_set_style_text_font(last_updated_label, &montserrat_regular_16, DEFAULT_SELECTOR);
};

void DeparturesScreen::updateDepartureItem(const std::string &trip_id, const std::string &line_text,
                                           const std::string &direction_text,
                                           const std::chrono::seconds &time_to_departure, bool is_cancelled) {
    if (panel == nullptr) {
        return;
    }

    const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(time_to_departure).count();
    std::string time_text = minutes <= 0 ? "Now" : std::to_string(minutes) + "'";

    auto it = departure_items.find(trip_id);
    if (it != departure_items.end()) {
        // Update existing item
        it->second.update(line_text, direction_text, time_text, time_to_departure, is_cancelled);
    } else {
        // Create new item
        DepartureItem &item = departure_items[trip_id];
        item.create(panel, line_text, direction_text, time_text, time_to_departure, is_cancelled);
    }
}

void DeparturesScreen::removeDepartureItem(const std::string &trip_id) {
    auto it = departure_items.find(trip_id);
    if (it != departure_items.end()) {
        it->second.destroy();
        departure_items.erase(it);
    }
}

void DeparturesScreen::addTextItem(const std::string &text) {
    if (panel == nullptr) {
        return;
    }

    {
        const ui_lock_guard lock;
        auto *item = lv_label_create(panel);
        lv_obj_set_width(item, lv_pct(100));
        lv_obj_set_align(item, LV_ALIGN_CENTER);
        lv_obj_set_style_text_font(item, &roboto_condensed_regular_28_4bpp, DEFAULT_SELECTOR);
        lv_label_set_text(item, text.c_str());
        lv_obj_set_style_text_font(item, &roboto_condensed_light_28_4bpp, DEFAULT_SELECTOR);
        lv_obj_set_style_text_color(item, Color::yellow, DEFAULT_SELECTOR);
    }
}

void DeparturesScreen::clean() {
    if (panel == nullptr) {
        return;
    }

    const ui_lock_guard lock;
    lv_obj_clean(panel);
    departure_items.clear();
};

void DeparturesScreen::cleanDepartureItems() {
    for (auto &pair : departure_items) {
        pair.second.destroy();
    }
    departure_items.clear();
}

void DeparturesScreen::updateLastUpdatedTime() {
    if (last_updated_label == nullptr) {
        return;
    }

    const ui_lock_guard lock;
    last_updated_time = std::chrono::system_clock::now();
    lv_label_set_text(last_updated_label, "Last updated: 0s ago");
}

void DeparturesScreen::refreshLastUpdatedDisplay() {
    if (last_updated_label == nullptr || last_updated_time.time_since_epoch().count() == 0) {
        return;
    }

    const ui_lock_guard lock;
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_updated_time);
    auto seconds = duration.count();

    std::string text;
    if (seconds < 60) {
        text = "Last updated: " + std::to_string(seconds) + "s ago";
    } else if (seconds < 3600) {
        auto minutes = seconds / 60;
        text = "Last updated: " + std::to_string(minutes) + "m ago";
    } else {
        auto hours = seconds / 3600;
        text = "Last updated: " + std::to_string(hours) + "h ago";
    }

    lv_label_set_text(last_updated_label, text.c_str());
}

void DeparturesScreen::reorderByDepartureTime() {
    if (panel == nullptr || departure_items.empty()) {
        return;
    }

    const ui_lock_guard lock;

    // Create a vector of (trip_id, departure_time) pairs for sorting
    std::vector<std::pair<std::string, std::optional<std::chrono::seconds>>> sorted_items;
    for (const auto &[trip_id, item] : departure_items) {
        sorted_items.emplace_back(trip_id, item.getDepartureTime());
    }

    // Sort by departure time (nullopt values go to the end)
    std::sort(sorted_items.begin(), sorted_items.end(), [](const auto &a, const auto &b) {
        if (!a.second && !b.second)
            return false; // Both nullopt, maintain original order
        if (!a.second)
            return false; // a is nullopt, b comes first
        if (!b.second)
            return true;                            // b is nullopt, a comes first
        return a.second.value() < b.second.value(); // Both have values, compare
    });

    // Reorder the LVGL objects by moving them to the correct positions
    for (size_t i = 0; i < sorted_items.size(); ++i) {
        const auto &trip_id = sorted_items[i].first;
        auto it = departure_items.find(trip_id);
        if (it != departure_items.end()) {
            lv_obj_t *item_obj = it->second.getItem();
            if (item_obj != nullptr) {
                lv_obj_move_to_index(item_obj, i);
            }
        }
    }
}

void UIManager::init() {
    // TODO Make use of the theme, allow switching between themes
    {
        const ui_lock_guard lock;
        auto *dispp = lv_disp_get_default();
        auto *theme = lv_theme_simple_init(dispp);
        lv_disp_set_theme(dispp, theme);
    }

    splash_screen.switchTo();
};

void SplashScreen::showInitializingWiFi() { updateStatus("Initializing WiFi..."); }

void SplashScreen::showStartingProvisioning() { updateStatus("Starting provisioning..."); }

void SplashScreen::showConnectingToWiFi() { updateStatus("Connecting to WiFi..."); }

void SplashScreen::showConnectingToWiFiWithResetButton(void (*reset_callback)()) {
    updateStatus("Connection taking longer than expected...");
    reset_callback_fn = reset_callback;

    // Hide the spinner when showing the reset button
    if (spinner != nullptr) {
        const ui_lock_guard lock;
        lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
    }

    if (reset_button == nullptr) {
        const ui_lock_guard lock;
        reset_button = lv_btn_create(screen);
        lv_obj_set_width(reset_button, 200);
        lv_obj_set_height(reset_button, 40);
        lv_obj_set_y(reset_button, -80);
        lv_obj_set_align(reset_button, LV_ALIGN_BOTTOM_MID);
        lv_obj_set_style_bg_color(reset_button, Color::white, DEFAULT_SELECTOR);

        lv_obj_t *reset_label = lv_label_create(reset_button);
        lv_label_set_text(reset_label, "Reset WiFi Settings");
        lv_obj_set_style_text_color(reset_label, Color::black, DEFAULT_SELECTOR);
        lv_obj_set_style_text_font(reset_label, &montserrat_regular_16, DEFAULT_SELECTOR);
        lv_obj_center(reset_label);

        lv_obj_add_event_cb(
            reset_button,
            [](lv_event_t *e) {
                SplashScreen *splash = (SplashScreen *)lv_event_get_user_data(e);
                if (splash->reset_callback_fn != nullptr) {
                    splash->reset_callback_fn();
                }
            },
            LV_EVENT_CLICKED, this);
    }
}

void SplashScreen::showConnectedSwitchingToMain() {
    updateStatus("Connected! Switching to departures screen...");

    // Hide reset button if it was shown
    if (reset_button != nullptr) {
        const ui_lock_guard lock;
        lv_obj_add_flag(reset_button, LV_OBJ_FLAG_HIDDEN);
    }
}

void ProvisioningScreen::showSetupInstructions() { addLine("It looks like you're trying to set up your device."); }

void ProvisioningScreen::showWiFiConnectedMessage() { addLine("Connected to WiFi! Switching to departures board..."); }

void ProvisioningScreen::showAppProvisioningInstructions() {
    addLine("Please download the \"ESP SoftAP Provisioning\" app from the App Store or Google Play, "
            "open it and follow the instructions.");
}

void DeparturesScreen::showLoadingMessage(const std::string &station_name) {
    addTextItem("Loading departures for " + station_name + "...");
}

void DeparturesScreen::showStationNotFoundError() {
    clean();
    addTextItem("Station not found.");
    addTextItem("Please access http://suntransit.local/ to configure your station.");
}
