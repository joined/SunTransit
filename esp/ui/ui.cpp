#include "ui.hpp"
#include <algorithm>
#include <vector>

namespace Color {
const lv_color_t black = lv_color_hex(0x000000);
const lv_color_t white = lv_color_hex(0xFFFFFF);
const lv_color_t yellow = lv_color_hex(0xFFFF00);
// BVG Product Colors (matching frontend)
const lv_color_t db_red = lv_color_hex(0xe21900);   // express, regional
const lv_color_t green = lv_color_hex(0x37874a);    // suburban (S-Bahn)
const lv_color_t blue = lv_color_hex(0x224f86);     // subway (U-Bahn)
const lv_color_t purple = lv_color_hex(0x993399);   // bus
const lv_color_t tram_red = lv_color_hex(0xcc0000); // tram
} // namespace Color

static constexpr lv_style_selector_t DEFAULT_SELECTOR = (uint32_t)LV_PART_MAIN | (uint32_t)LV_STATE_DEFAULT;

// Helper functions
static void setup_flex_container(lv_obj_t *obj, lv_flex_flow_t flow, lv_flex_align_t main_align = LV_FLEX_ALIGN_START,
                                 lv_flex_align_t cross_align = LV_FLEX_ALIGN_START,
                                 lv_flex_align_t track_align = LV_FLEX_ALIGN_START) {
    lv_obj_set_flex_flow(obj, flow);
    lv_obj_set_flex_align(obj, main_align, cross_align, track_align);
}

static void remove_borders_and_padding(lv_obj_t *obj) {
    lv_obj_set_style_radius(obj, 0, DEFAULT_SELECTOR);
    lv_obj_set_style_border_width(obj, 0, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_all(obj, 0, DEFAULT_SELECTOR);
}

void Screen::switchTo(lv_scr_load_anim_t anim_type, uint32_t time, uint32_t delay) {
    if (screen == nullptr) {
        this->init();
    }

    {
        const ui_lock_guard lock;
        lv_scr_load_anim(screen, anim_type, time, delay, true);
    }
}

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
    lv_obj_set_size(spinner, 50, 51);
    lv_obj_set_y(spinner, 60);
    lv_obj_set_align(spinner, LV_ALIGN_CENTER);
    lv_obj_set_style_arc_color(spinner, Color::yellow, (uint32_t)LV_PART_INDICATOR | (uint32_t)LV_STATE_DEFAULT);
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

    panel = lv_obj_create(screen);
    lv_obj_set_size(panel, lv_pct(100), lv_pct(85));
    lv_obj_set_y(panel, 5);
    lv_obj_set_align(panel, LV_ALIGN_CENTER);
    setup_flex_container(panel, LV_FLEX_FLOW_COLUMN);
    remove_borders_and_padding(panel);
    lv_obj_set_style_pad_hor(panel, 10, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_ver(panel, 5, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_row(panel, 2, DEFAULT_SELECTOR);
    lv_obj_set_y(panel, 0);
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
    lv_obj_set_size(log_line, lv_pct(100), LV_SIZE_CONTENT);
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

lv_color_t DepartureItem::getProductColor(const std::string &product_type) {
    if (product_type == "bus") {
        return Color::purple;
    } else if (product_type == "tram") {
        return Color::tram_red;
    } else if (product_type == "suburban") {
        return Color::green;
    } else if (product_type == "subway") {
        return Color::blue;
    } else if (product_type == "ferry") {
        return Color::blue; // Same as subway for now
    } else if (product_type == "express" || product_type == "regional") {
        return Color::db_red;
    }
    return Color::black; // Default fallback
}

void DepartureItem::create(lv_obj_t *parent, const std::string &line_text, const std::string &direction_text,
                           const std::string &time_text, const std::chrono::seconds &time_to_departure,
                           const std::string &product_type, bool is_cancelled) {
    const ui_lock_guard lock;
    departure_time = time_to_departure;

    item = lv_obj_create(parent);
    lv_obj_set_size(item, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(item, LV_OPA_0, DEFAULT_SELECTOR);
    lv_obj_set_style_border_width(item, 0, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_top(item, 2, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_bottom(item, 2, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_left(item, 2, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_right(item, 8, DEFAULT_SELECTOR);
    lv_obj_set_style_text_color(item, Color::black, DEFAULT_SELECTOR);
    lv_obj_set_style_text_font(item, &roboto_condensed_regular_28_4bpp, DEFAULT_SELECTOR);
    setup_flex_container(item, LV_FLEX_FLOW_ROW, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

    // Create colored badge for line (fixed width)
    lv_obj_t *line_badge = lv_obj_create(item);
    lv_obj_set_size(line_badge, 60, 30);
    lv_obj_set_scroll_dir(line_badge, LV_DIR_NONE);
    lv_obj_set_style_radius(line_badge, 5, DEFAULT_SELECTOR);
    lv_obj_set_style_border_width(line_badge, 0, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_all(line_badge, 0, DEFAULT_SELECTOR);
    lv_obj_set_style_bg_color(line_badge, getProductColor(product_type), DEFAULT_SELECTOR);

    line = lv_label_create(line_badge);
    lv_obj_center(line);
    lv_label_set_text(line, line_text.c_str());
    lv_obj_set_style_text_color(line, Color::white, DEFAULT_SELECTOR);
    lv_obj_set_style_text_font(line, &roboto_condensed_regular_28_4bpp, DEFAULT_SELECTOR);

    // Direction takes remaining space
    direction = lv_label_create(item);
    lv_obj_set_height(direction,
                      33); // ideally we'd specify "one line" here but we can't, the value is guessed visually
    lv_obj_set_style_flex_grow(direction, 1, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_left(
        direction, 9,
        DEFAULT_SELECTOR); // Add spacing from line badge (60px line + 9px padding = 69px, matching header)
    lv_label_set_long_mode(direction, LV_LABEL_LONG_DOT);
    lv_label_set_text(direction, direction_text.c_str());
    lv_obj_set_style_text_font(direction, &roboto_condensed_light_28_4bpp, DEFAULT_SELECTOR);

    // Time column (fixed width, right-aligned)
    time = lv_label_create(item);
    lv_label_set_text(time, time_text.c_str());
    lv_obj_set_style_text_align(time, LV_TEXT_ALIGN_RIGHT, DEFAULT_SELECTOR);

    applyStrikethroughStyle(is_cancelled);
}

void DepartureItem::update(const std::string &line_text, const std::string &direction_text,
                           const std::string &time_text, const std::chrono::seconds &time_to_departure,
                           const std::string &product_type, bool is_cancelled) {
    if (item == nullptr) {
        return;
    }

    const ui_lock_guard lock;
    departure_time = time_to_departure;
    lv_label_set_text(time, time_text.c_str());
    applyStrikethroughStyle(is_cancelled);
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
    strikethrough_line = nullptr;
}

void DepartureItem::applyStrikethroughStyle(bool enable) {
    if (item == nullptr) {
        return;
    }

    const ui_lock_guard lock;

    if (enable) {
        if (strikethrough_line == nullptr) {
            // Create the strikethrough line - position it absolutely to avoid flexbox interference
            strikethrough_line = lv_obj_create(item);
            lv_obj_set_size(strikethrough_line, lv_pct(100), 2);
            lv_obj_set_pos(strikethrough_line, 0, 16);
            lv_obj_set_style_bg_color(strikethrough_line, Color::black, DEFAULT_SELECTOR);
            remove_borders_and_padding(strikethrough_line);
            // Remove from flex layout so it doesn't interfere
            lv_obj_add_flag(strikethrough_line, LV_OBJ_FLAG_IGNORE_LAYOUT);
        } else {
            // Show existing line and update position
            lv_obj_clear_flag(strikethrough_line, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(strikethrough_line, 0, 16);
        }
    } else {
        if (strikethrough_line != nullptr) {
            // Hide the line
            lv_obj_add_flag(strikethrough_line, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void DeparturesScreen::init() {
    const ui_lock_guard lock;
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, Color::black, DEFAULT_SELECTOR);
    setup_flex_container(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(screen, 0, DEFAULT_SELECTOR);

    // Header with fixed height using horizontal flexbox
    header = lv_obj_create(screen);
    lv_obj_set_size(header, lv_pct(100), 35);
    lv_obj_set_style_bg_color(header, Color::black, DEFAULT_SELECTOR);
    lv_obj_set_style_text_font(header, &roboto_condensed_light_28_4bpp, DEFAULT_SELECTOR);
    lv_obj_set_style_text_color(header, Color::white, DEFAULT_SELECTOR);
    lv_obj_set_style_border_width(header, 0, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_left(header, 12, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_right(header, 15, DEFAULT_SELECTOR);
    lv_obj_set_style_flex_grow(header, 0, DEFAULT_SELECTOR);
    setup_flex_container(header, LV_FLEX_FLOW_ROW, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    line = lv_label_create(header);
    lv_obj_set_width(line, 68);
    lv_label_set_text(line, "Line");

    direction = lv_label_create(header);
    lv_obj_set_style_flex_grow(direction, 1, DEFAULT_SELECTOR);
    lv_label_set_text(direction, "Direction");

    departure = lv_label_create(header);
    lv_label_set_text(departure, "ETD");
    lv_obj_set_style_text_align(departure, LV_TEXT_ALIGN_RIGHT, DEFAULT_SELECTOR);

    // Panel takes remaining space (flex-grow)
    panel = lv_obj_create(screen);
    lv_obj_set_width(panel, lv_pct(100));
    lv_obj_set_style_flex_grow(panel, 1, DEFAULT_SELECTOR);
    setup_flex_container(panel, LV_FLEX_FLOW_COLUMN);
    remove_borders_and_padding(panel);
    lv_obj_set_style_pad_hor(panel, 10, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_ver(panel, 5, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_row(panel, 2, DEFAULT_SELECTOR);
    lv_obj_set_style_bg_color(panel, Color::white, DEFAULT_SELECTOR);

    // Footer with fixed height
    footer = lv_obj_create(screen);
    lv_obj_set_size(footer, lv_pct(100), 30);
    lv_obj_set_style_bg_color(footer, Color::black, DEFAULT_SELECTOR);
    lv_obj_set_style_border_width(footer, 0, DEFAULT_SELECTOR);
    lv_obj_set_style_pad_all(footer, 0, DEFAULT_SELECTOR);
    lv_obj_set_style_flex_grow(footer, 0, DEFAULT_SELECTOR);

    // Move last updated label to footer
    last_updated_label = lv_label_create(footer);
    lv_obj_set_align(last_updated_label, LV_ALIGN_LEFT_MID);
    lv_obj_set_x(last_updated_label, 10);
    lv_label_set_text(last_updated_label, "Last updated: --");
    lv_obj_set_style_text_color(last_updated_label, Color::white, DEFAULT_SELECTOR);
    lv_obj_set_style_text_font(last_updated_label, &montserrat_regular_16, DEFAULT_SELECTOR);
};

void DeparturesScreen::updateDepartureItem(const std::string &trip_id, const std::string &line_text,
                                           const std::string &direction_text,
                                           const std::chrono::seconds &time_to_departure,
                                           const std::string &product_type, bool is_cancelled) {
    if (panel == nullptr) {
        return;
    }

    const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(time_to_departure).count();
    std::string time_text = minutes <= 0 ? "Now" : std::to_string(minutes) + "'";

    auto it = departure_items.find(trip_id);
    if (it != departure_items.end()) {
        // Update existing item
        it->second.update(line_text, direction_text, time_text, time_to_departure, product_type, is_cancelled);
    } else {
        // Create new item
        DepartureItem &item = departure_items[trip_id];
        item.create(panel, line_text, direction_text, time_text, time_to_departure, product_type, is_cancelled);
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
        lv_label_set_text(item, text.c_str());
        lv_obj_set_style_text_font(item, &roboto_condensed_light_28_4bpp, DEFAULT_SELECTOR);
        lv_obj_set_style_text_color(item, Color::black, DEFAULT_SELECTOR);
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
        lv_obj_set_size(reset_button, 200, 40);
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
