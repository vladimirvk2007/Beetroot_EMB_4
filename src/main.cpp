#include <Arduino.h>

#define ADC_IN 4
#define LED_OUT 15

#define VOLTAGE_THRESHOLD_MV 1300
#define HYSTERESIS_MV 200

#define ADC_SAMPLES 16
#define LOOP_DELAY_MS 500

uint32_t readMilliVoltsAverage(uint8_t samples) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < samples; ++i) {
        sum += analogReadMilliVolts(ADC_IN);
    }
    return sum / samples;
}


void setup() {
    Serial.begin(115200);
    analogReadResolution(12);
    pinMode(ADC_IN, INPUT);
    pinMode(LED_OUT, OUTPUT);

    Serial.println();
    Serial.println("ESP32 ADC + LED threshold control");
    Serial.printf("Threshold: %.2f V, hysteresis: +/- %.2f V\n",
                  VOLTAGE_THRESHOLD_MV / 1000.0f,
                  HYSTERESIS_MV / 1000.0f);
    Serial.println("Voltage(V) | LED");
}

void loop() {
    static bool ledState = false;

    const uint32_t voltageMilliVolts = readMilliVoltsAverage(ADC_SAMPLES);
    const float voltage = voltageMilliVolts / 1000.0f;

    const uint32_t lowThresholdMilliVolts = VOLTAGE_THRESHOLD_MV - HYSTERESIS_MV;
    const uint32_t highThresholdMilliVolts = VOLTAGE_THRESHOLD_MV + HYSTERESIS_MV;

    if (voltageMilliVolts < lowThresholdMilliVolts) {
        ledState = true;
    } else if (voltageMilliVolts > highThresholdMilliVolts) {
        ledState = false;
    }
    digitalWrite(LED_OUT, ledState ? HIGH : LOW);

    Serial.printf("%9.3f | %s\n", voltage, ledState ? "ON" : "OFF");

    delay(LOOP_DELAY_MS);
}
