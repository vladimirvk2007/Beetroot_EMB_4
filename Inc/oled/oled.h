#ifndef OLED_H
#define OLED_H

#include <stdbool.h>
#include <stdint.h>

#include "i2c/i2c.h"
#include "oled/oled_font.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Дескриптор дисплея (непрозорий). */
typedef struct OLED_HandleTypeDef OLED_HandleTypeDef;

/* Розмір панелі. */
typedef enum
{
  OLED_PANEL_128X32,
  OLED_PANEL_128X64,
  OLED_PANEL_128X128,
} OLED_PanelTypeDef;

/* Параметри дисплея. */
typedef struct
{
  uint16_t address;         /* 7-бітна I2C-адреса, 0 = 0x3C */
  uint32_t scl_speed_hz;    /* частота SCL, Гц; 0 = не змінювати швидкість шини */
  OLED_PanelTypeDef panel;  /* розмір панелі */
  bool flip;                /* true = повернути зображення на 180° */
} OLED_SettingsTypeDef;

/* Типові параметри для панелі 128x64. */
#define OLED_SETTINGS_128X64_DEFAULT() \
  { 0U, 0U, OLED_PANEL_128X64, false }

/*
 * Ініціалізує дисплей на готовій шині I2C (після I2C_Init).
 * Ненульова scl_speed_hz переналаштовує всю шину (до 400 кГц).
 * Пам'ять під дескриптор і буфер кадру береться з купи.
 * Усі функції повертають HAL_ERROR при хибних аргументах,
 * інакше - статус HAL невдалої передачі I2C.
 */
HAL_StatusTypeDef OLED_Init(I2C_HandleTypeDef *hi2c,
                            const OLED_SettingsTypeDef *settings,
                            OLED_HandleTypeDef **oled);

/* Видаляє дисплей і звільняє пам'ять. */
HAL_StatusTypeDef OLED_Deinit(OLED_HandleTypeDef *oled);

/* Очищає екран. */
HAL_StatusTypeDef OLED_Clear(OLED_HandleTypeDef *oled);

/*
 * Примітиви; координати в пікселях, (0, 0) - верхній лівий кут.
 * invert = true гасить пікселі. Частини за межами панелі відсікаються.
 * width і height прямокутників - точні розміри в пікселях.
 */
HAL_StatusTypeDef OLED_DrawPixel(OLED_HandleTypeDef *oled, uint8_t x, uint8_t y, bool invert);
HAL_StatusTypeDef OLED_DrawLine(OLED_HandleTypeDef *oled, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool invert);
HAL_StatusTypeDef OLED_DrawCircle(OLED_HandleTypeDef *oled, uint8_t x, uint8_t y, uint8_t radius, bool invert);
HAL_StatusTypeDef OLED_FillCircle(OLED_HandleTypeDef *oled, uint8_t x, uint8_t y, uint8_t radius, bool invert);
HAL_StatusTypeDef OLED_DrawRectangle(OLED_HandleTypeDef *oled, uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool invert);
HAL_StatusTypeDef OLED_FillRectangle(OLED_HandleTypeDef *oled, uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool invert);

/* Яскравість: 0..255. */
HAL_StatusTypeDef OLED_SetContrast(OLED_HandleTypeDef *oled, uint8_t contrast);

/* Вмикає (on = true) або вимикає (сплячий режим) дисплей. */
HAL_StatusTypeDef OLED_Power(OLED_HandleTypeDef *oled, bool on);

/*
 * Текст у рядку сторінки (висота рядка 8 пікселів).
 * row    - номер рядка, 0 = верхній
 * scale  - розмір: 1 = 8x8 (до 16 символів), 2 = 16x16 (до 8), 3 = 24x24 (до 5);
 *          x2 займає 2 рядки, x3 - 3; довший текст або вихід за панель = HAL_ERROR
 * invert - true = інвертовані кольори
 */
HAL_StatusTypeDef OLED_Print(OLED_HandleTypeDef *oled, uint8_t row, uint8_t scale, bool invert, const char *text);

/* Те саме з форматуванням printf (без інверсії, до 32 символів). */
HAL_StatusTypeDef OLED_Printf(OLED_HandleTypeDef *oled, uint8_t row, uint8_t scale, const char *fmt, ...)
    __attribute__((format(printf, 4, 5)));

/* Текст BDF-шрифтом (напр. OLED_BdfFontNenr12_21x26) у позиції x, y в пікселях. */
HAL_StatusTypeDef OLED_PrintBdf(OLED_HandleTypeDef *oled, const uint8_t *font, int x, int y, const char *text);

#ifdef __cplusplus
}
#endif

#endif /* OLED_H */
