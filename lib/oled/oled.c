#include "oled.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp_check.h"
#include "esp_log.h"
#include "ssd1306.h"

#define OLED_DEFAULT_ADDRESS 0x3C
#define OLED_DEFAULT_SPEED_HZ 400000U
#define OLED_PRINTF_BUF 32

static const char *TAG = "oled";

/* Внутрішній контекст дисплея. */
struct oled_s {
    ssd1306_handle_t dev;
};

/* Перетворює розмір панелі модуля на тип бібліотеки ssd1306. */
static ssd1306_panel_sizes_t to_panel(oled_panel_t panel)
{
    switch (panel) {
    case OLED_PANEL_128X32:
        return SSD1306_PANEL_128x32;
    case OLED_PANEL_128X128:
        return SSD1306_PANEL_128x128;
    default:
        return SSD1306_PANEL_128x64;
    }
}

/* Перевіряє аргументи, підставляє типові значення та ініціалізує драйвер SSD1306. */
esp_err_t oled_init(i2c_master_bus_handle_t bus, const oled_settings_t *settings, oled_handle_t *oled)
{
    if (bus == NULL || settings == NULL || oled == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ssd1306_config_t cfg = {
        .i2c_address = settings->address ? settings->address : OLED_DEFAULT_ADDRESS,
        .i2c_clock_speed = settings->scl_speed_hz ? settings->scl_speed_hz : OLED_DEFAULT_SPEED_HZ,
        .panel_size = to_panel(settings->panel),
        .offset_x = 0,
        .flip_enabled = settings->flip,
        .display_enabled = true,
    };

    struct oled_s *ctx = calloc(1, sizeof(*ctx));
    if (ctx == NULL) {
        return ESP_ERR_NO_MEM;
    }

    const esp_err_t err = ssd1306_init(bus, &cfg, &ctx->dev);
    if (err != ESP_OK || ctx->dev == NULL) {
        ESP_LOGE(TAG, "SSD1306 init failed: %s", esp_err_to_name(err));
        free(ctx);
        return err != ESP_OK ? err : ESP_FAIL;
    }

    *oled = ctx;
    return ESP_OK;
}

/* Видаляє пристрій SSD1306 і звільняє контекст. */
esp_err_t oled_deinit(oled_handle_t oled)
{
    if (oled == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    const esp_err_t err = ssd1306_delete(oled->dev);
    free(oled);
    return err;
}

/* Очищає екран. */
esp_err_t oled_clear(oled_handle_t oled)
{
    return oled ? ssd1306_clear_display(oled->dev, false) : ESP_ERR_INVALID_ARG;
}

/* Вмикає один піксель за координатами x/y та оновлює дисплей. */
esp_err_t oled_draw_pixel(oled_handle_t oled, uint8_t x, uint8_t y, bool invert)
{
    if (oled == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_RETURN_ON_ERROR(ssd1306_set_pixel(oled->dev, x, y, invert), TAG, "set pixel failed");
    return ssd1306_display_pages(oled->dev);
}

/* Малює лінію між точками (x0, y0) та (x1, y1), після чого оновлює дисплей. */
esp_err_t oled_draw_line(oled_handle_t oled, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool invert)
{
    if (oled == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_RETURN_ON_ERROR(ssd1306_set_line(oled->dev, x0, y0, x1, y1, invert), TAG, "set line failed");
    return ssd1306_display_pages(oled->dev);
}

/* Малює контур кола з центром (x, y) і заданим радіусом. */
esp_err_t oled_draw_circle(oled_handle_t oled, uint8_t x, uint8_t y, uint8_t radius, bool invert)
{
    if (oled == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_RETURN_ON_ERROR(ssd1306_set_circle(oled->dev, x, y, radius, invert), TAG, "set circle failed");
    return ssd1306_display_pages(oled->dev);
}

/* Малює заповнене коло з центром (x, y) і заданим радіусом. */
esp_err_t oled_fill_circle(oled_handle_t oled, uint8_t x, uint8_t y, uint8_t radius, bool invert)
{
    return oled ? ssd1306_display_filled_circle(oled->dev, x, y, radius, invert) : ESP_ERR_INVALID_ARG;
}

/* Малює контур прямокутника від точки (x, y) заданої ширини та висоти. */
esp_err_t oled_draw_rectangle(oled_handle_t oled, uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool invert)
{
    if (oled == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_RETURN_ON_ERROR(ssd1306_set_rectangle(oled->dev, x, y, width, height, invert), TAG, "set rectangle failed");
    return ssd1306_display_pages(oled->dev);
}

/* Малює заповнений прямокутник від точки (x, y) заданої ширини та висоти. */
esp_err_t oled_fill_rectangle(oled_handle_t oled, uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool invert)
{
    return oled ? ssd1306_display_filled_rectangle(oled->dev, x, y, width, height, invert) : ESP_ERR_INVALID_ARG;
}

/* Встановлює яскравість (0..255). */
esp_err_t oled_set_contrast(oled_handle_t oled, uint8_t contrast)
{
    return oled ? ssd1306_set_contrast(oled->dev, contrast) : ESP_ERR_INVALID_ARG;
}

/* Вмикає або вимикає дисплей. */
esp_err_t oled_power(oled_handle_t oled, bool on)
{
    if (oled == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return on ? ssd1306_enable_display(oled->dev) : ssd1306_disable_display(oled->dev);
}

/* Текст у рядку сторінки; scale вибирає шрифт 8x8, 16x16 або 24x24. */
esp_err_t oled_print(oled_handle_t oled, uint8_t row, uint8_t scale, bool invert, const char *text)
{
    if (oled == NULL || text == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err;
    switch (scale) {
    case 1:
        err = ssd1306_display_text(oled->dev, row, text, invert);
        break;
    case 2:
        err = ssd1306_display_text_x2(oled->dev, row, text, invert);
        break;
    case 3:
        err = ssd1306_display_text_x3(oled->dev, row, text, invert);
        break;
    default:
        err = ESP_ERR_INVALID_ARG;
        break;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "print failed (row %u, scale %u, \"%s\"): %s", row, scale, text, esp_err_to_name(err));
    }
    return err;
}

/* Форматує рядок у буфер (обрізає до OLED_PRINTF_BUF) і виводить через oled_print. */
esp_err_t oled_printf(oled_handle_t oled, uint8_t row, uint8_t scale, const char *fmt, ...)
{
    char buf[OLED_PRINTF_BUF];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    return oled_print(oled, row, scale, false, buf);
}

/* Текст BDF-шрифтом у піксельних координатах. */
esp_err_t oled_print_bdf(oled_handle_t oled, const uint8_t *font, int x, int y, const char *text)
{
    if (oled == NULL || font == NULL || text == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return ssd1306_display_bdf_text(oled->dev, font, text, x, y);
}

