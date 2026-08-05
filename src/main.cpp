#include <Arduino.h>

#define LED_OUT 15

bool ledState = false;

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(LED_OUT, OUTPUT);
    digitalWrite(LED_OUT, LOW);
}

void loop() {
    if (ledState) {
        digitalWrite(LED_OUT, LOW);
        ledState = false;
        Serial.println("LED OFF");
    } else {
        digitalWrite(LED_OUT, HIGH);
        ledState = true;
        Serial.println("LED ON");
    }

    delay(1000);
}
