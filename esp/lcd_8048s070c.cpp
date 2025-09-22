#include <driver/gpio.h>
#include <driver/i2c.h>
#include <driver/ledc.h>
#include <esp_check.h>
#include <esp_err.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_touch_gt911.h>
#include <esp_log.h>
#include <esp_lvgl_port.h>

#include "lcd_8048s070c.hpp"

/* LCD size */
#define LCD_HRES 800
#define LCD_VRES 480

/* LCD pins */
#define PIN_LCD_HSYNC (GPIO_NUM_39)
#define PIN_LCD_VSYNC (GPIO_NUM_40)
#define PIN_LCD_DE (GPIO_NUM_41)
#define PIN_LCD_PCLK (GPIO_NUM_42)
#define PIN_BL (GPIO_NUM_2)

/* Touch settings */
#define TOUCH_GT911_I2C_NUM (I2C_NUM_0)
#define TOUCH_GT911_I2C_CLK_HZ (400000)

/* LCD touch pins */
#define TOUCH_GT911_I2C_SCL (GPIO_NUM_20)
#define TOUCH_GT911_I2C_SDA (GPIO_NUM_19)
#define PIN_TOUCH_RST (GPIO_NUM_38)

static const char *TAG = "LCD";

/* LCD IO and panel */
static esp_lcd_panel_handle_t lcd_panel = NULL;
static esp_lcd_touch_handle_t touch_handle = NULL;

/* LVGL display and touch */
static lv_disp_t *lvgl_disp = NULL;
static lv_indev_t *lvgl_touch_indev = NULL;

namespace LVGL_LCD {

static void backlight_init(int gpio, uint32_t duty_percent) {
    ledc_timer_config_t tcfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 20000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&tcfg));

    ledc_channel_config_t c = {
        .gpio_num = gpio,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
        .flags = {.output_invert = 0},
    };
    ESP_ERROR_CHECK(ledc_channel_config(&c));

    uint32_t duty = (duty_percent * ((1 << 10) - 1)) / 100;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}

static esp_err_t app_lcd_init(void) {
    /* LCD backlight */
    backlight_init(PIN_BL, 50);

    ESP_LOGD(TAG, "Install LCD driver");
    // See e.g. https://github.com/MMlodzinski/Sunton_ESP32-8048S070c_ESP_IDF_LVGL_example/blob/main/example/main/bsp.h
    const esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_PLL160M,
        .timings =
            {
                .pclk_hz = 20 * 1000 * 1000,
                .h_res = LCD_HRES,
                .v_res = LCD_VRES,
                .hsync_pulse_width = 30,
                .hsync_back_porch = 16,
                .hsync_front_porch = 20,
                .vsync_pulse_width = 13,
                .vsync_back_porch = 10,
                .vsync_front_porch = 22,
                .flags =
                    {
                        .pclk_active_neg = 1,
                    },
            },
        .data_width = 16,
        .bits_per_pixel = 16,
        .num_fbs = 2,                           // double buffer for RGB displays
        .bounce_buffer_size_px = LCD_HRES * 10, // small bounce buffer for 800x480
        .dma_burst_size = 64,
        .hsync_gpio_num = PIN_LCD_HSYNC,
        .vsync_gpio_num = PIN_LCD_VSYNC,
        .de_gpio_num = PIN_LCD_DE,
        .pclk_gpio_num = PIN_LCD_PCLK,
        .disp_gpio_num = -1,
        .data_gpio_nums = {15, 7, 6, 5, 4, 9, 46, 3, 8, 16, 1, 14, 21, 47, 48, 45},
        .flags =
            {
                .fb_in_psram = 1,
            },
    };

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &lcd_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcd_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(lcd_panel));
    return ESP_OK;
}

static esp_err_t app_touch_init(void) {
    /* Initialize I2C */
    const i2c_config_t i2c_conf = {.mode = I2C_MODE_MASTER,
                                   .sda_io_num = TOUCH_GT911_I2C_SDA,
                                   .scl_io_num = TOUCH_GT911_I2C_SCL,
                                   .sda_pullup_en = GPIO_PULLUP_DISABLE,
                                   .scl_pullup_en = GPIO_PULLUP_DISABLE,
                                   .master = {.clk_speed = TOUCH_GT911_I2C_CLK_HZ}};
    ESP_RETURN_ON_ERROR(i2c_param_config(TOUCH_GT911_I2C_NUM, &i2c_conf), TAG, "I2C configuration failed");
    ESP_RETURN_ON_ERROR(i2c_driver_install(TOUCH_GT911_I2C_NUM, i2c_conf.mode, 0, 0, 0), TAG,
                        "I2C initialization failed");

    /* Initialize touch HW */
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_HRES,
        .y_max = LCD_VRES,
        .rst_gpio_num = PIN_TOUCH_RST,
        // Touch interrupt is unusable due to routing mistake, see https://esp3d.io/ESP3D-TFT/Version_1.X/hardware/esp32-s3/sunton-70-8048/
        .int_gpio_num = GPIO_NUM_NC,
        .levels =
            {
                .reset = 0,
                .interrupt = 0,
            },
        .flags =
            {
                .swap_xy = 0,
                .mirror_x = 0,
                .mirror_y = 0,
            },
    };
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    const esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c((uint32_t)TOUCH_GT911_I2C_NUM, &tp_io_config, &tp_io_handle), TAG, "");
    return esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &touch_handle);
}

static esp_err_t app_lvgl_init(void) {
    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "LVGL port initialization failed");

    /* Add LCD screen */
    ESP_LOGD(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .panel_handle = lcd_panel,
        .buffer_size = LCD_HRES * LCD_VRES,
        .hres = LCD_HRES,
        .vres = LCD_VRES,
        .monochrome = false,
        /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
        .rotation =
            {
                .swap_xy = false,
                .mirror_x = false,
                .mirror_y = false,
            },
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags =
            {
                .buff_dma = true,
                .buff_spiram = false,
                .swap_bytes = false,
                .direct_mode = true,
            },
    };

    const lvgl_port_display_rgb_cfg_t rgb_cfg = {.flags = {
                                                     .bb_mode = true,
                                                     .avoid_tearing = true,
                                                 }};

    lvgl_disp = lvgl_port_add_disp_rgb(&disp_cfg, &rgb_cfg);

    /* Add touch input (for selected screen) */
    ESP_LOGD(TAG, "Add touch input");
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = lvgl_disp,
        .handle = touch_handle,
    };

    lvgl_touch_indev = lvgl_port_add_touch(&touch_cfg);

    return ESP_OK;
}

esp_err_t init(void) {
    /* LCD HW initialization */
    ESP_ERROR_CHECK(app_lcd_init());

    /* Touch initialization */
    ESP_ERROR_CHECK(app_touch_init());

    /* LVGL initialization */
    ESP_ERROR_CHECK(app_lvgl_init());

    // TODO We probably need some locking mechanism when accessing the NVS for this board
    // At the moment, when writing it the NVS, the screen flickers

    return ESP_OK;
}
} // namespace LVGL_LCD
