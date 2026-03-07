#include <Arduino.h>

#define POT_PIN 4    // GPIO4 - потенціометр (ADC)
#define PWM_PIN 15   // GPIO15 - вихід ШІМ

void setup() {
    Serial.begin(115200);
    pinMode(PWM_PIN, OUTPUT);
    analogReadResolution(12); // 12 біт
    analogSetAttenuation(ADC_11db); // максимальний діапазон
}

void loop() {
    // Зчитуємо значення з потенціометра (12 біт, 0..4095)
    int adc = analogRead(POT_PIN);
    // Перетворюємо у 10 градацій (0..10)
    int level = map(adc, 0, 4095, 0, 10);

    // Виводимо значення в серійний монітор
    Serial.printf("ADC: %d  Level: %d\n", adc, level);

    // Програмний ШІМ: 10 тактів (градацій)
    for (int i = 0; i < 10; ++i) {
        if (i < level) {
            digitalWrite(PWM_PIN, HIGH);
        } else {
            digitalWrite(PWM_PIN, LOW);
        }
        delayMicroseconds(100); // Тривалість одного такту (можна підібрати)
    }
}

