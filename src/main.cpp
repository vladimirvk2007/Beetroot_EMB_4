#include <Arduino.h>

#define LED_RED_OUTPUT      15
#define LED_GREEN_OUTPUT    16
#define EXT_BUTTON_IN       17
#define BOOT_BUTTON_IN      0

#define DELAY_FAST_MS       100
#define DELAY_SLOW_MS       500
#define DELAY_EXTRA_MS      1000
#define DEBOUNCE_DELAY_MS   150
#define SMART_DELAY_STEP_MS 10

enum Mode {
    FAST_MODE,  // EXT_BUTTON
    SLOW_MODE,  // BOOT_BUTTON_IN
    EXTRA_MODE  // EXT_BUTTON + BOOT_BUTTON_IN
};

Mode currentMode = SLOW_MODE;

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(LED_RED_OUTPUT, OUTPUT);
    pinMode(LED_GREEN_OUTPUT, OUTPUT);
    digitalWrite(LED_RED_OUTPUT, LOW);
    digitalWrite(LED_GREEN_OUTPUT, LOW);

    pinMode(EXT_BUTTON_IN, INPUT_PULLUP);
    pinMode(BOOT_BUTTON_IN, INPUT_PULLUP);
}

void checkButtons() {
    int extState  = digitalRead(EXT_BUTTON_IN);
    int bootState = digitalRead(BOOT_BUTTON_IN);

    if (extState == LOW && bootState == LOW) {
        delay(DEBOUNCE_DELAY_MS);
        if (currentMode != EXTRA_MODE) {
            currentMode = EXTRA_MODE;
            Serial.println("Extra mode, delay = 1000 ms");
        }
    } else if (extState == LOW) {
        delay(DEBOUNCE_DELAY_MS);
        if (currentMode != FAST_MODE) {
            currentMode = FAST_MODE;
            Serial.println("Fast mode, delay = 100 ms");
        }
    } else if (bootState == LOW) {
        delay(DEBOUNCE_DELAY_MS);
        if (currentMode != SLOW_MODE) {
            currentMode = SLOW_MODE;
            Serial.println("Slow mode, delay = 500 ms");
        }
    }
}

void smartDelay(uint32_t totalMs) {
    for (uint32_t elapsed = 0; elapsed < totalMs; elapsed += SMART_DELAY_STEP_MS) {
        checkButtons();
        delay(SMART_DELAY_STEP_MS);
    }
}

void loop() {
    uint32_t currentDelay = DELAY_SLOW_MS;

    if (currentMode == FAST_MODE) {
        currentDelay = DELAY_FAST_MS;
    } else if (currentMode == EXTRA_MODE) {
        currentDelay = DELAY_EXTRA_MS;
    }

    // Red LED ON, Green LED OFF
    digitalWrite(LED_RED_OUTPUT, HIGH);
    digitalWrite(LED_GREEN_OUTPUT, LOW);
    smartDelay(currentDelay);

    // Red LED OFF, Green LED ON
    digitalWrite(LED_RED_OUTPUT, LOW);
    digitalWrite(LED_GREEN_OUTPUT, HIGH);
    smartDelay(currentDelay);
}
