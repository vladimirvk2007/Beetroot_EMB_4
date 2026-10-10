#ifndef OLED_H
#define OLED_H

#include <stdbool.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Дескриптор дисплея (непрозорий). */
typedef struct oled_s *oled_handle_t;

/* Розмір панелі дисплея. */
typedef enum {
    OLED_PANEL_128X32,
    OLED_PANEL_128X64,
    OLED_PANEL_128X128,
} oled_panel_t;

/* Параметри дисплея. */
typedef struct {
    uint16_t address;      /* I2C-адреса, 0 = 0x3C */
    uint32_t scl_speed_hz; /* частота SCL, Гц; 0 = 400 кГц */
    oled_panel_t panel;    /* розмір панелі */
    bool flip;             /* true = повернути зображення на 180° */
} oled_settings_t;

/* Типові параметри для панелі 128x64. */
#define OLED_SETTINGS_128X64_DEFAULT() \
    { .address = 0, .scl_speed_hz = 0, .panel = OLED_PANEL_128X64, .flip = false }

/*
 * Ініціалізує дисплей на шині I2C.
 * bus      - дескриптор шини (з i2c_bus_init)
 * settings - параметри дисплея
 * oled     - [out] дескриптор створеного дисплея
 */
esp_err_t oled_init(i2c_master_bus_handle_t bus, const oled_settings_t *settings, oled_handle_t *oled);

/* Видаляє дисплей і звільняє пам'ять. oled - дескриптор після oled_init. */
esp_err_t oled_deinit(oled_handle_t oled);

/* Очищає екран. */
esp_err_t oled_clear(oled_handle_t oled);

/* Малює примітиви; координати в пікселях, (0, 0) — верхній лівий кут. */
esp_err_t oled_draw_pixel(oled_handle_t oled, uint8_t x, uint8_t y, bool invert);
esp_err_t oled_draw_line(oled_handle_t oled, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool invert);
esp_err_t oled_draw_circle(oled_handle_t oled, uint8_t x, uint8_t y, uint8_t radius, bool invert);
esp_err_t oled_fill_circle(oled_handle_t oled, uint8_t x, uint8_t y, uint8_t radius, bool invert);
esp_err_t oled_draw_rectangle(oled_handle_t oled, uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool invert);
esp_err_t oled_fill_rectangle(oled_handle_t oled, uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool invert);

/* Встановлює яскравість. contrast: 0..255. */
esp_err_t oled_set_contrast(oled_handle_t oled, uint8_t contrast);

/* Вмикає (on = true) або вимикає (сплячий режим) дисплей. */
esp_err_t oled_power(oled_handle_t oled, bool on);

/*
 * Виводить текст у рядок сторінки (висота рядка 8 пікселів).
 * row    - номер рядка (сторінки), 0 = верхній
 * scale  - розмір шрифту: 1 = 8x8 (до 18 символів), 2 = 16x16 (до 8), 3 = 24x24 (до 5);
 *           x2 займає 2 рядки, x3 - 3; довший текст дає ESP_ERR_INVALID_SIZE
 * invert - true = інвертовані кольори
 * text   - рядок для виводу
 */
esp_err_t oled_print(oled_handle_t oled, uint8_t row, uint8_t scale, bool invert, const char *text);

/*
 * Те саме, що oled_print, але з форматуванням як у printf
 * (без інверсії, результат обмежено 32 символами).
 */
esp_err_t oled_printf(oled_handle_t oled, uint8_t row, uint8_t scale, const char *fmt, ...)
    __attribute__((format(printf, 4, 5)));

/*
 * Виводить текст BDF-шрифтом у довільній позиції.
 * font - масив BDF-шрифту, напр. з font_nenr12_21x26.h
 * x, y - координати в пікселях
 * text - рядок для виводу
 */
esp_err_t oled_print_bdf(oled_handle_t oled, const uint8_t *font, int x, int y, const char *text);

#ifdef __cplusplus
}
#endif

#endif


