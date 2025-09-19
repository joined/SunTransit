#include <driver/gpio.h>
#include <driver/i2c.h>
#include <driver/spi_master.h>
#include <esp_check.h>
#include <esp_err.h>
#include <esp_freertos_hooks.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_st7796.h>
#include <esp_lcd_touch_gt911.h>
#include <esp_log.h>
#include <esp_lvgl_port.h>
#include <freertos/task.h>
#include <mutex>

#include "lcd.hpp"

/* LCD size */
#define LCD_ST7796_H_RES (480)
#define LCD_ST7796_V_RES (320)

/* LCD settings */
#define LCD_ST7796_SPI_NUM (SPI2_HOST)
#define LCD_ST7796_PIXEL_CLK_HZ (80 * 1000 * 1000)
#define LCD_ST7796_CMD_BITS (8)
#define LCD_ST7796_PARAM_BITS (8)
#define LCD_ST7796_ENDIAN (LCD_RGB_ENDIAN_BGR)
#define LCD_ST7796_BITS_PER_PIXEL (16)
#define LCD_ST7796_DRAW_BUFF_DOUBLE (0)
#define LCD_ST7796_DRAW_BUFF_HEIGHT (32)
#define LCD_ST7796_BL_ON_LEVEL (1)

/* LCD pins */
#define LCD_ST7796_GPIO_SCLK (GPIO_NUM_14)
#define LCD_ST7796_GPIO_MOSI (GPIO_NUM_13)
#define LCD_ST7796_GPIO_DC (GPIO_NUM_2)
#define LCD_ST7796_GPIO_CS (GPIO_NUM_15)
#define LCD_ST7796_GPIO_BL (GPIO_NUM_27)

/* Touch settings */
#define TOUCH_GT911_I2C_NUM (I2C_NUM_0)
#define TOUCH_GT911_I2C_CLK_HZ (400000)

/* LCD touch pins */
#define TOUCH_GT911_I2C_SCL (GPIO_NUM_32)
#define TOUCH_GT911_I2C_SDA (GPIO_NUM_33)
#define TOUCH_GT911_GPIO_INT (GPIO_NUM_21)

static const char *TAG = "LCD";

/* LCD IO and panel */
static esp_lcd_panel_io_handle_t lcd_io = NULL;
static esp_lcd_panel_handle_t lcd_panel = NULL;
static esp_lcd_touch_handle_t touch_handle = NULL;

/* LVGL display and touch */
static lv_disp_t *lvgl_disp = NULL;
static lv_indev_t *lvgl_touch_indev = NULL;

namespace LVGL_LCD {

static esp_err_t app_lcd_init(void) {
    esp_err_t ret = ESP_OK;

    /* LCD backlight */
    gpio_config_t bk_gpio_config = {
        .pin_bit_mask = 1ULL << LCD_ST7796_GPIO_BL,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    /* LCD initialization */
    ESP_LOGD(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = {
        .mosi_io_num = LCD_ST7796_GPIO_MOSI,
        .miso_io_num = GPIO_NUM_NC,
        .sclk_io_num = LCD_ST7796_GPIO_SCLK,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = LCD_ST7796_H_RES * LCD_ST7796_DRAW_BUFF_HEIGHT * sizeof(uint16_t),
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(LCD_ST7796_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI init failed");

    ESP_LOGD(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = LCD_ST7796_GPIO_CS,
        .dc_gpio_num = LCD_ST7796_GPIO_DC,
        .spi_mode = 0,
        .pclk_hz = LCD_ST7796_PIXEL_CLK_HZ,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = LCD_ST7796_CMD_BITS,
        .lcd_param_bits = LCD_ST7796_PARAM_BITS,
    };
    const esp_err_t newPanelIoRet =
        esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_ST7796_SPI_NUM, &io_config, &lcd_io);
    if (unlikely(newPanelIoRet != ESP_OK)) {
        ESP_LOGE(TAG, "New panel IO failed");
        if (lcd_io) {
            esp_lcd_panel_io_del(lcd_io);
        }
        spi_bus_free(LCD_ST7796_SPI_NUM);
        return newPanelIoRet;
    }

    ESP_LOGD(TAG, "Install LCD driver");
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = GPIO_NUM_NC,
        .rgb_endian = LCD_ST7796_ENDIAN,
        .bits_per_pixel = LCD_ST7796_BITS_PER_PIXEL,
    };
    const esp_err_t newPanelRet = esp_lcd_new_panel_st7796(lcd_io, &panel_config, &lcd_panel);
    if (unlikely(newPanelRet != ESP_OK)) {
        ESP_LOGE(TAG, "New panel failed");
        if (lcd_panel) {
            esp_lcd_panel_del(lcd_panel);
        }
        if (lcd_io) {
            esp_lcd_panel_io_del(lcd_io);
        }
        spi_bus_free(LCD_ST7796_SPI_NUM);
        return newPanelRet;
    }

    esp_lcd_panel_reset(lcd_panel);
    esp_lcd_panel_init(lcd_panel);
    esp_lcd_panel_swap_xy(lcd_panel, true);
    esp_lcd_panel_disp_on_off(lcd_panel, true);

    /* LCD backlight on */
    ESP_ERROR_CHECK(gpio_set_level(LCD_ST7796_GPIO_BL, LCD_ST7796_BL_ON_LEVEL));

    return ret;
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
        // The driver doesn't swap these internally when `swap_xy` is set, we need to do it manually.
        .x_max = LCD_ST7796_V_RES,
        .y_max = LCD_ST7796_H_RES,
        .rst_gpio_num = GPIO_NUM_NC, // Shared with LCD reset
        // Touch interrupt is unusable due to routing mistake, see https://esp3d.io/ESP3D-TFT/Version_1.X/hardware/esp32/sunton-35-3248/
        .int_gpio_num = GPIO_NUM_NC,
        .levels =
            {
                .reset = 0,
                .interrupt = 0,
            },
        .flags =
            {
                .swap_xy = 1,
                .mirror_x = 1,
                .mirror_y = 0,
            },
    };
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    const esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)TOUCH_GT911_I2C_NUM, &tp_io_config, &tp_io_handle), TAG, "");
    return esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &touch_handle);
}

static esp_err_t app_lvgl_init(void) {
    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_cfg.task_stack = 1024 * 6;
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "LVGL port initialization failed");

    /* Add LCD screen */
    ESP_LOGD(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = lcd_io,
        .panel_handle = lcd_panel,
        .buffer_size = LCD_ST7796_H_RES * LCD_ST7796_DRAW_BUFF_HEIGHT,
        .double_buffer = LCD_ST7796_DRAW_BUFF_DOUBLE,
        .hres = LCD_ST7796_H_RES,
        .vres = LCD_ST7796_V_RES,
        .monochrome = false,
        /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
        .rotation =
            {
                .swap_xy = true,
                .mirror_x = false,
                .mirror_y = false,
            },
        .flags = {
            .buff_dma = true,
            .swap_bytes = true,
        }};
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);

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

    return ESP_OK;
}
} // namespace LVGL_LCD
