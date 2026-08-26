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
    bool activeLevel;          // true: HIGH = ON, false: LOW = ON
    ledMode_t mode;            // LED_MODE_OFF, LED_MODE_ON, LED_MODE_BLINK
    ledOutputState_t output;   // Current logical output state
    uint32_t onTimeMs;         // Blinking ON time in ms
    uint32_t offTimeMs;        // Blinking OFF time in ms
    uint32_t lastToggleTime;   // Timestamp of last blink transition
    bool isConfigured;         // Flag indicating if this LED slot is active
} ledItem_t;

typedef struct {
    ledItem_t leds[LED_CONTROL_MAX_LEDS];
    uint8_t count;
} ledControl_t;

/**
 * @brief Initializes the LED control context.
 * @param ctx Pointer to ledControl_t context structure.
 * @return 0 on success, -1 on error.
 */
int ledControl_init(ledControl_t *ctx);

/**
 * @brief Deinitializes the LED control context, turns off all configured LEDs.
 * @param ctx Pointer to ledControl_t context structure.
 * @return 0 on success, -1 on error.
 */
int ledControl_deinit(ledControl_t *ctx);

/**
 * @brief Adds a new LED to the control context.
 * @param ctx Pointer to ledControl_t context structure.
 * @param pin GPIO pin number for the LED.
 * @param activeLevel Active level logic (true = HIGH turns ON, false = LOW turns ON).
 * @param ledId Output pointer to receive assigned LED ID (0 to LED_CONTROL_MAX_LEDS - 1).
 * @return 0 on success, -1 on error.
 */
int ledControl_addLed(ledControl_t *ctx, uint8_t pin, bool activeLevel, uint8_t *ledId);

/**
 * @brief Sets mode of a specific LED.
 * @param ctx Pointer to ledControl_t context structure.
 * @param ledId ID of the LED.
 * @param mode Desired mode (LED_MODE_OFF, LED_MODE_ON, LED_MODE_BLINK).
 * @return 0 on success, -1 on error.
 */
int ledControl_setMode(ledControl_t *ctx, uint8_t ledId, ledMode_t mode);

/**
 * @brief Configures ON and OFF times for blinking and activates BLINK mode.
 * @param ctx Pointer to ledControl_t context structure.
 * @param ledId ID of the LED.
 * @param onTimeMs ON duration in milliseconds.
 * @param offTimeMs OFF duration in milliseconds.
 * @return 0 on success, -1 on error.
 */
int ledControl_setBlink(ledControl_t *ctx, uint8_t ledId, uint32_t onTimeMs, uint32_t offTimeMs);

/**
 * @brief Updates state of all LEDs in the control context. Must be called periodically in loop().
 * @param ctx Pointer to ledControl_t context structure.
 * @return 0 on success, -1 on error.
 */
int ledControl_update(ledControl_t *ctx);

/**
 * @brief Gets current mode of a specific LED.
 * @param ctx Pointer to ledControl_t context structure.
 * @param ledId ID of the LED.
 * @param mode Output pointer to store current mode.
 * @return 0 on success, -1 on error.
 */
int ledControl_getMode(ledControl_t *ctx, uint8_t ledId, ledMode_t *mode);

/**
 * @brief Toggles LED state between ON and OFF (or switches from BLINK to OFF).
 * @param ctx Pointer to ledControl_t context structure.
 * @param ledId ID of the LED.
 * @return 0 on success, -1 on error.
 */
int ledControl_toggle(ledControl_t *ctx, uint8_t ledId);

#endif // LED_CONTROL_H
