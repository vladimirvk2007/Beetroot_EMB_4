#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <Arduino.h>

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
    bool activeLevel;
    ledMode_t mode;
    ledOutputState_t output;
    uint32_t onTimeMs;
    uint32_t offTimeMs;
    uint32_t lastToggleTime;
    bool isConfigured;
} ledControl_t;

int ledControl_init(ledControl_t *ctx, uint8_t pin, bool activeLevel,
                    ledMode_t mode, uint32_t onTimeMs, uint32_t offTimeMs);

int ledControl_deinit(ledControl_t *ctx);

int ledControl_update(ledControl_t *ctx);

#endif // LED_CONTROL_H
