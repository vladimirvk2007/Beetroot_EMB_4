#include <Arduino.h>

#define BUTTON_PIN 4
#define LED_SLOW_BLINK_DELAY 1000
#define LED_FAST_BLINK_DELAY 200

enum BlinkState {
    SLOW_BLINK,
    FAST_BLINK,
};

int button_state(int gpio_pin);
void led_blink(enum BlinkState blink_mode);


void setup() {
    // Start the Serial Monitor at 115200 baud
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
    int button_state = HIGH; // Initialize button state
    enum BlinkState blink_mode = SLOW_BLINK;
    bool fast_blink = false;

    button_state = button_state(BUTTON_PIN);
    if (button_state == LOW) {
        Serial.println("Button Pressed!");
        blink_mode = FAST_BLINK;
    }

    led_blink(blink_mode);
}

int button_state(int gpio_pin) {
    if (digitalRead(gpio_pin) == LOW) {
        //Serial.println("Button Pressed!");
        delay(50); // Debounce delay

        while (digitalRead(gpio_pin) == LOW) {
            // Wait for the button to be released
            delay(10);
        }

        return LOW; // Button is pressed

        //Serial.println("Button Released!");
    } else {
        return HIGH; // Button is released
    }
}

void led_blink(enum BlinkState blink_mode) {
    int delay_time = 0;

    if (blink_mode == SLOW_BLINK) {
        delay_time = LED_SLOW_BLINK_DELAY;
    } else if (blink_mode == FAST_BLINK) {
        delay_time = LED_FAST_BLINK_DELAY;
    }

    digitalWrite(LED_BUILTIN, LOW);
    delay(delay_time);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(delay_time);
}
