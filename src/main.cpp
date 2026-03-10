#include <Arduino.h>

#define LED_PIN PC13
void setup() {
    pinMode(LED_PIN, OUTPUT);
    Serial.begin(115200);
    while (!Serial) {
    }
    Serial.println("UART ready!\r\n");
}

void loop() {
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
    Serial.println("LED blinked!");
}

