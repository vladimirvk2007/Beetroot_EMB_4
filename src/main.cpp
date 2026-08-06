#include <Arduino.h>

#define BUTTON_IN  16
#define LED_OUT    15

void setup() {
    Serial.begin(115200);
    delay(1000); // Очікування відкриття монітора порту

    pinMode(LED_OUT, OUTPUT);
    pinMode(BUTTON_IN, INPUT_PULLUP);
    digitalWrite(LED_OUT, LOW);
}

void loop() {
    int buttonState = digitalRead(BUTTON_IN);

    if (buttonState == LOW) {
        digitalWrite(LED_OUT, HIGH);
        Serial.println("Button pressed");
    } else {
        digitalWrite(LED_OUT, LOW);
        Serial.println("Button released");
    }

    delay(100);
}
