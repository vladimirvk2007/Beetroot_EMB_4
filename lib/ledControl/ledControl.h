#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <Arduino.h>

#define LED_CONTROL_MAX_LEDS 10

typedef enum {
    LED_MODE_OFF = 0,
    LED_MODE_ON,
    LED_MODE_BLINK
} ledMode_t;

typedef enum {
    LED_OUTPUT_OFF = 0,
    LED_OUTPUT_ON
} ledOutputState_t;

typedef struct {
    uint8_t pin;
    bool activeLevel;          // true: HIGH = УВІМК, false: LOW = УВІМК
    ledMode_t mode;            // LED_MODE_OFF, LED_MODE_ON, LED_MODE_BLINK
    ledOutputState_t output;   // Поточний логічний стан виходу
    uint32_t onTimeMs;         // Час увімкнення при блиманні у мс
    uint32_t offTimeMs;        // Час вимкнення при блиманні у мс
    uint32_t lastToggleTime;   // Часова мітка останнього перемикання блимання
    bool isConfigured;         // Прапор, що вказує, чи активний цей слот світлодіода
} ledItem_t;

typedef struct {
    ledItem_t leds[LED_CONTROL_MAX_LEDS];
    uint8_t count;
} ledControl_t;

/**
 * @brief Ініціалізує контекст керування світлодіодами.
 * @param ctx Вказівник на структуру контексту ledControl_t.
 * @return 0 у разі успіху, -1 у разі помилки.
 */
int ledControl_init(ledControl_t *ctx);

/**
 * @brief Деініціалізує контекст керування світлодіодами, вимикає всі налаштовані світлодіоди.
 * @param ctx Вказівник на структуру контексту ledControl_t.
 * @return 0 у разі успіху, -1 у разі помилки.
 */
int ledControl_deinit(ledControl_t *ctx);

/**
 * @brief Додає новий світлодіод до контексту керування.
 * @param ctx Вказівник на структуру контексту ledControl_t.
 * @param pin Номер GPIO піна для світлодіода.
 * @param activeLevel Логіка активного рівня (true = HIGH вмикає, false = LOW вмикає).
 * @param ledId Вихідний вказівник для отримання призначеного ID (від 0 до LED_CONTROL_MAX_LEDS - 1).
 * @return 0 у разі успіху, -1 у разі помилки.
 */
int ledControl_addLed(ledControl_t *ctx, uint8_t pin, bool activeLevel, uint8_t *ledId);

/**
 * @brief Встановлює режим роботи конкретного світлодіода.
 * @param ctx Вказівник на структуру контексту ledControl_t.
 * @param ledId ID світлодіода.
 * @param mode Бажаний режим (LED_MODE_OFF, LED_MODE_ON, LED_MODE_BLINK).
 * @return 0 у разі успіху, -1 у разі помилки.
 */
int ledControl_setMode(ledControl_t *ctx, uint8_t ledId, ledMode_t mode);

/**
 * @brief Налаштовує час увімкнення та вимкнення для блимання й активує режим BLINK.
 * @param ctx Вказівник на структуру контексту ledControl_t.
 * @param ledId ID світлодіода.
 * @param onTimeMs Тривалість увімкненого стану в мілісекундах.
 * @param offTimeMs Тривалість вимкненого стану в мілісекундах.
 * @return 0 у разі успіху, -1 у разі помилки.
 */
int ledControl_setBlink(ledControl_t *ctx, uint8_t ledId, uint32_t onTimeMs, uint32_t offTimeMs);

/**
 * @brief Оновлює стан усіх світлодіодів у контексті. Має регулярно викликатися в loop().
 * @param ctx Вказівник на структуру контексту ledControl_t.
 * @return 0 у разі успіху, -1 у разі помилки.
 */
int ledControl_update(ledControl_t *ctx);

/**
 * @brief Отримує поточний режим конкретного світлодіода.
 * @param ctx Вказівник на структуру контексту ledControl_t.
 * @param ledId ID світлодіода.
 * @param mode Вихідний вказівник для збереження поточного режиму.
 * @return 0 у разі успіху, -1 у разі помилки.
 */
int ledControl_getMode(ledControl_t *ctx, uint8_t ledId, ledMode_t *mode);

/**
 * @brief Перемикає стан світлодіода між увімкненим та вимкненим (або з режиму BLINK у вимкнений).
 * @param ctx Вказівник на структуру контексту ledControl_t.
 * @param ledId ID світлодіода.
 * @return 0 у разі успіху, -1 у разі помилки.
 */
int ledControl_toggle(ledControl_t *ctx, uint8_t ledId);

#endif // LED_CONTROL_H
