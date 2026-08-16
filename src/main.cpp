#include <Arduino.h>

#define LED_PIN PC13
#define ADC_PIN A0
#define ADC_SAMPLES 16
#define ADC_RESOLUTION_BITS 12
#define ADC_MAX_CODE ((1u << ADC_RESOLUTION_BITS) - 1)
#define ADC_REFERENCE_MV 3300

uint16_t readAveragedAdc(uint8_t sampleCount) {
    if (sampleCount == 0u) {
        return 0;
    }

    uint32_t sum = 0;

    for (uint8_t i = 0; i < sampleCount; ++i) {
        sum += analogRead(ADC_PIN);
    }

    return (uint16_t)(sum / sampleCount);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    analogReadResolution(ADC_RESOLUTION_BITS);
    pinMode(LED_PIN, OUTPUT);
    pinMode(ADC_PIN, INPUT_ANALOG);

    Serial.println("RAW | mV");
}

void loop() {
    const uint16_t raw = readAveragedAdc(ADC_SAMPLES);
    const float millivolts = raw * ADC_REFERENCE_MV  / (float)ADC_MAX_CODE;

    digitalWrite(LED_PIN, HIGH);
    Serial.print(raw);
    Serial.print(" | ");
    Serial.print(millivolts);
    Serial.println(" mV");
    delay(500);

    digitalWrite(LED_PIN, LOW);
    delay(500);
}
