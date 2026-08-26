#include "ledControl.h"

static void applyPinState(uint8_t pin, bool activeLevel, ledOutputState_t output) {
    if (activeLevel) {
        digitalWrite(pin, (output == LED_OUTPUT_ON) ? HIGH : LOW);
    } else {
        digitalWrite(pin, (output == LED_OUTPUT_ON) ? LOW : HIGH);
    }
}

int ledControl_init(ledControl_t *ctx) {
    if (ctx == NULL) {
        Serial.println("Error [ledControl]: Context pointer is NULL");
        return -1;
    }

    ctx->count = 0;
    for (int i = 0; i < LED_CONTROL_MAX_LEDS; i++) {
        ctx->leds[i].pin = 0;
        ctx->leds[i].activeLevel = true;
        ctx->leds[i].mode = LED_MODE_OFF;
        ctx->leds[i].output = LED_OUTPUT_OFF;
        ctx->leds[i].onTimeMs = 0;
        ctx->leds[i].offTimeMs = 0;
        ctx->leds[i].lastToggleTime = 0;
        ctx->leds[i].isConfigured = false;
    }

    return 0;
}

int ledControl_deinit(ledControl_t *ctx) {
    if (ctx == NULL) {
        Serial.println("Error [ledControl]: Context pointer is NULL");
        return -1;
    }

    for (int i = 0; i < LED_CONTROL_MAX_LEDS; i++) {
        if (ctx->leds[i].isConfigured) {
            applyPinState(ctx->leds[i].pin, ctx->leds[i].activeLevel, LED_OUTPUT_OFF);
            ctx->leds[i].isConfigured = false;
        }
    }

    ctx->count = 0;
    return 0;
}

int ledControl_addLed(ledControl_t *ctx, uint8_t pin, bool activeLevel, uint8_t *ledId) {
    if (ctx == NULL) {
        Serial.println("Error [ledControl]: Context pointer is NULL");
        return -1;
    }

    if (ctx->count >= LED_CONTROL_MAX_LEDS) {
        Serial.println("Error [ledControl]: Maximum LED count reached");
        return -1;
    }

    uint8_t index = ctx->count;

    ctx->leds[index].pin = pin;
    ctx->leds[index].activeLevel = activeLevel;
    ctx->leds[index].mode = LED_MODE_OFF;
    ctx->leds[index].output = LED_OUTPUT_OFF;
    ctx->leds[index].onTimeMs = 500;
    ctx->leds[index].offTimeMs = 500;
    ctx->leds[index].lastToggleTime = 0;
    ctx->leds[index].isConfigured = true;

    pinMode(pin, OUTPUT);
    applyPinState(pin, activeLevel, LED_OUTPUT_OFF);

    if (ledId != NULL) {
        *ledId = index;
    }

    ctx->count++;
    return 0;
}

int ledControl_setMode(ledControl_t *ctx, uint8_t ledId, ledMode_t mode) {
    if (ctx == NULL) {
        Serial.println("Error [ledControl]: Context pointer is NULL");
        return -1;
    }

    if (ledId >= LED_CONTROL_MAX_LEDS || !ctx->leds[ledId].isConfigured) {
        Serial.println("Error [ledControl]: Invalid LED ID");
        return -1;
    }

    ledItem_t *led = &ctx->leds[ledId];
    led->mode = mode;

    switch (mode) {
        case LED_MODE_OFF:
            led->output = LED_OUTPUT_OFF;
            applyPinState(led->pin, led->activeLevel, LED_OUTPUT_OFF);
            break;
        case LED_MODE_ON:
            led->output = LED_OUTPUT_ON;
            applyPinState(led->pin, led->activeLevel, LED_OUTPUT_ON);
            break;
        case LED_MODE_BLINK:
            led->output = LED_OUTPUT_ON;
            led->lastToggleTime = millis();
            applyPinState(led->pin, led->activeLevel, LED_OUTPUT_ON);
            break;
    }

    return 0;
}

int ledControl_setBlink(ledControl_t *ctx, uint8_t ledId, uint32_t onTimeMs, uint32_t offTimeMs) {
    if (ctx == NULL) {
        Serial.println("Error [ledControl]: Context pointer is NULL");
        return -1;
    }

    if (ledId >= LED_CONTROL_MAX_LEDS || !ctx->leds[ledId].isConfigured) {
        Serial.println("Error [ledControl]: Invalid LED ID");
        return -1;
    }

    ctx->leds[ledId].onTimeMs = onTimeMs;
    ctx->leds[ledId].offTimeMs = offTimeMs;

    return ledControl_setMode(ctx, ledId, LED_MODE_BLINK);
}

int ledControl_update(ledControl_t *ctx) {
    if (ctx == NULL) {
        Serial.println("Error [ledControl]: Context pointer is NULL");
        return -1;
    }

    uint32_t now = millis();

    for (int i = 0; i < LED_CONTROL_MAX_LEDS; i++) {
        ledItem_t *led = &ctx->leds[i];

        if (!led->isConfigured || led->mode != LED_MODE_BLINK) {
            continue;
        }

        if (led->output == LED_OUTPUT_ON) {
            if (now - led->lastToggleTime >= led->onTimeMs) {
                led->output = LED_OUTPUT_OFF;
                led->lastToggleTime = now;
                applyPinState(led->pin, led->activeLevel, LED_OUTPUT_OFF);
            }
        } else {
            if (now - led->lastToggleTime >= led->offTimeMs) {
                led->output = LED_OUTPUT_ON;
                led->lastToggleTime = now;
                applyPinState(led->pin, led->activeLevel, LED_OUTPUT_ON);
            }
        }
    }

    return 0;
}

int ledControl_getMode(ledControl_t *ctx, uint8_t ledId, ledMode_t *mode) {
    if (ctx == NULL || mode == NULL) {
        Serial.println("Error [ledControl]: Invalid pointer");
        return -1;
    }

    if (ledId >= LED_CONTROL_MAX_LEDS || !ctx->leds[ledId].isConfigured) {
        Serial.println("Error [ledControl]: Invalid LED ID");
        return -1;
    }

    *mode = ctx->leds[ledId].mode;
    return 0;
}

int ledControl_toggle(ledControl_t *ctx, uint8_t ledId) {
    if (ctx == NULL) {
        Serial.println("Error [ledControl]: Context pointer is NULL");
        return -1;
    }

    if (ledId >= LED_CONTROL_MAX_LEDS || !ctx->leds[ledId].isConfigured) {
        Serial.println("Error [ledControl]: Invalid LED ID");
        return -1;
    }

    if (ctx->leds[ledId].mode == LED_MODE_ON) {
        return ledControl_setMode(ctx, ledId, LED_MODE_OFF);
    } else {
        return ledControl_setMode(ctx, ledId, LED_MODE_ON);
    }
}
