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
        const std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
        lv_scr_load_anim(screen, anim_type, time, delay, true);
    }
}

lv_obj_t *Screen::createPanel(lv_scroll_snap_t snap_type) {
    const std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    auto *panel = lv_obj_create(screen);
    lv_obj_set_width(panel, lv_pct(100));
    lv_obj_set_height(panel, 250);
    lv_obj_set_y(panel, 5);
    lv_obj_set_align(panel, LV_ALIGN_CENTER);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scroll_snap_y(panel, snap_type);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_set_style_radius(panel, 0, DEFAULT_SELECTOR);
    lv_obj_set_style_border_width(panel, 0, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_hor(panel, 10, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_ver(panel, 5, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_row(panel, 2, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_column(panel, 0, DEFAULT_SELECTOR);
    return panel;
};

void SplashScreen::init() {
    const std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
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

    spinner = lv_spinner_create(screen, 1000, 90);
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

    const std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    lv_label_set_text(status, message.c_str());
};

void ProvisioningScreen::init() {
    const std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, Color::black, DEFAULT_SELECTOR);
    lv_obj_set_style_bg_opa(screen, 255, DEFAULT_SELECTOR);

    panel = createPanel(LV_SCROLL_SNAP_END);
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

    const std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    auto *log_line = lv_label_create(panel);
    lv_obj_set_width(log_line, lv_pct(100));
    lv_obj_set_height(log_line, LV_SIZE_CONTENT);
    lv_obj_set_align(log_line, LV_ALIGN_TOP_LEFT);
    lv_label_set_text(log_line, message.c_str());
    lv_obj_set_style_text_font(log_line, &montserrat_regular_16, DEFAULT_SELECTOR);

    lv_obj_scroll_to_y(panel, LV_COORD_MAX, LV_ANIM_OFF);

    if (lv_obj_get_child_cnt(screen) > MAX_LINES) {
        lv_obj_del(lv_obj_get_child(screen, 0));
    }
};

void ProvisioningScreen::addQRCode(const std::string &data, const int size) {
    if (panel == nullptr) {
        return;
    }

    const std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    auto *qrcode = lv_qrcode_create(panel, size, Color::black, Color::white);
    lv_qrcode_update(qrcode, data.c_str(), data.length());

    lv_obj_scroll_to_y(panel, LV_COORD_MAX, LV_ANIM_OFF);
};

void DepartureItem::create(lv_obj_t *parent, const std::string &line_text, const std::string &direction_text,
                           const std::string &time_text, const std::optional<std::chrono::seconds> &time_to_departure) {
    const std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
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
                           const std::string &time_text, const std::optional<std::chrono::seconds> &time_to_departure) {
    if (!isValid()) {
        return;
    }

    const std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    departure_time = time_to_departure;
    lv_label_set_text(line, line_text.c_str());
    lv_label_set_text(direction, direction_text.c_str());
    lv_label_set_long_mode(direction, direction_text.length() > 30 ? LV_LABEL_LONG_SCROLL : LV_LABEL_LONG_DOT);
    lv_label_set_text(time, time_text.c_str());
}

void DepartureItem::destroy() {
    if (!isValid()) {
        return;
    }

    const std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    lv_obj_del(item);
    item = nullptr;
    line = nullptr;
    direction = nullptr;
    time = nullptr;
}

void DeparturesScreen::init() {
    const std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, Color::black, DEFAULT_SELECTOR);

    line = lv_label_create(screen);
    lv_obj_set_x(line, 9);
    lv_obj_set_y(line, 7);
    lv_label_set_text(line, "Line");
    lv_obj_set_style_text_font(line, &roboto_condensed_light_28_4bpp, DEFAULT_SELECTOR);

    direction = lv_label_create(line);
    lv_obj_set_x(direction, 66);
    lv_label_set_text(direction, "Direction");

    departure = lv_label_create(line);
    lv_obj_set_x(departure, 410);
    lv_label_set_text(departure, "ETD");

    panel = createPanel(LV_SCROLL_SNAP_START);
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
                                           const std::optional<std::chrono::seconds> &time_to_departure) {
    if (panel == nullptr) {
        return;
    }

    std::string time_text;
    if (time_to_departure.has_value()) {
        const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(time_to_departure.value()).count();
        time_text = minutes <= 0 ? "Now" : std::to_string(minutes) + "'";
    } else {
        // TODO Use more appropriate symbol for cancelled trips
        time_text = "=";
    }

    auto it = departure_items.find(trip_id);
    if (it != departure_items.end()) {
        // Update existing item
        it->second.update(line_text, direction_text, time_text, time_to_departure);
    } else {
        // Create new item
        DepartureItem &item = departure_items[trip_id];
        item.create(panel, line_text, direction_text, time_text, time_to_departure);
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
        const std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
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

    const std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
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

    const std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
    last_updated_time = std::chrono::system_clock::now();
    lv_label_set_text(last_updated_label, "Last updated: 0s ago");
}

void DeparturesScreen::refreshLastUpdatedDisplay() {
    if (last_updated_label == nullptr || last_updated_time.time_since_epoch().count() == 0) {
        return;
    }

    const std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
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

    const std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);

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
        const std::lock_guard<std::recursive_mutex> lock(lvgl_mutex);
        auto *dispp = lv_disp_get_default();
        auto *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                            true, LV_FONT_DEFAULT);
        lv_disp_set_theme(dispp, theme);
    }

    splash_screen.switchTo();
};

void SplashScreen::showInitializingWiFi() { updateStatus("Initializing WiFi..."); }

void SplashScreen::showStartingProvisioning() { updateStatus("Starting provisioning..."); }

void SplashScreen::showConnectingToWiFi() { updateStatus("Connecting to WiFi..."); }

void SplashScreen::showConnectedSwitchingToMain() { updateStatus("Connected! Switching to departures screen..."); }

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
    cleanDepartureItems();
    addTextItem("Station not found.");
    addTextItem("Please access http://suntransit.local/ to configure your station.");
}
