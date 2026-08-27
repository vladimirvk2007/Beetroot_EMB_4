#include "ledControl.h"

static void applyPinState(uint8_t pin, bool activeLevel, ledOutputState_t output) {
    if (activeLevel) {
        digitalWrite(pin, (output == LED_OUTPUT_ON) ? HIGH : LOW);
    } else {
        digitalWrite(pin, (output == LED_OUTPUT_ON) ? LOW : HIGH);
    }
}

int ledControl_init(ledControl_t *ctx, uint8_t pin, bool activeLevel,
                    ledMode_t mode, uint32_t onTimeMs, uint32_t offTimeMs) {
    if (ctx == NULL) {
        Serial.println("Error [ledControl]: Context pointer is NULL");
        return -1;
    }

    bool preserveBlinkPhase = ctx->isConfigured &&
                              ctx->mode == LED_MODE_BLINK &&
                              mode == LED_MODE_BLINK;

    ctx->pin = pin;
    ctx->activeLevel = activeLevel;
    ctx->mode = LED_MODE_OFF;
    if (!preserveBlinkPhase) {
        ctx->output = LED_OUTPUT_OFF;
    }
    ctx->onTimeMs = onTimeMs;
    ctx->offTimeMs = offTimeMs;
    if (!preserveBlinkPhase) {
        ctx->lastToggleTime = 0;
    }
    ctx->isConfigured = true;

    pinMode(pin, OUTPUT);
    ctx->mode = mode;

    switch (mode) {
        case LED_MODE_OFF:
            ctx->output = LED_OUTPUT_OFF;
            applyPinState(ctx->pin, ctx->activeLevel, LED_OUTPUT_OFF);
            break;
        case LED_MODE_ON:
            ctx->output = LED_OUTPUT_ON;
            applyPinState(ctx->pin, ctx->activeLevel, LED_OUTPUT_ON);
            break;
        case LED_MODE_BLINK:
            if (!preserveBlinkPhase) {
                ctx->output = LED_OUTPUT_ON;
                ctx->lastToggleTime = millis();
            }
            applyPinState(ctx->pin, ctx->activeLevel, ctx->output);
            break;
        default:
            ctx->isConfigured = false;
            return -1;
    }

    return 0;
}

int ledControl_deinit(ledControl_t *ctx) {
    if (ctx == NULL) {
        Serial.println("Error [ledControl]: Context pointer is NULL");
        return -1;
    }

    if (ctx->isConfigured) {
        applyPinState(ctx->pin, ctx->activeLevel, LED_OUTPUT_OFF);
        ctx->isConfigured = false;
    }

    return 0;
}

int ledControl_update(ledControl_t *ctx) {
    if (ctx == NULL) {
        Serial.println("Error [ledControl]: Context pointer is NULL");
        return -1;
    }

    uint32_t now = millis();

    if (!ctx->isConfigured || ctx->mode != LED_MODE_BLINK) {
        return 0;
    }

    if (ctx->output == LED_OUTPUT_ON) {
        if (now - ctx->lastToggleTime >= ctx->onTimeMs) {
            ctx->output = LED_OUTPUT_OFF;
            ctx->lastToggleTime = now;
            applyPinState(ctx->pin, ctx->activeLevel, LED_OUTPUT_OFF);
        }
    } else if (now - ctx->lastToggleTime >= ctx->offTimeMs) {
        ctx->output = LED_OUTPUT_ON;
        ctx->lastToggleTime = now;
        applyPinState(ctx->pin, ctx->activeLevel, LED_OUTPUT_ON);
    }

    return 0;
}
