#include <Arduino.h>

#define ADC_PIN 4

#define ADC_MAX_CODE 4095
#define DIVIDER_TOP_R_OHM 10000
#define DIVIDER_BOT_R_OHM 1000
#define SAMPLES_PER_POINT 64
#define ATTENUATION_SETTLE_DELAY_MS 40
#define LOOP_DELAY_MS 10000

constexpr float kDividerRatio =
    (DIVIDER_TOP_R_OHM + DIVIDER_BOT_R_OHM) / (float)(DIVIDER_BOT_R_OHM);

struct AttenuationConfig {
    adc_attenuation_t atten;
    const char* name;
    float fsVoltage;
};

constexpr AttenuationConfig kAttenuations[] = {
    {ADC_0db, "0 dB", 0.95f},
    {ADC_2_5db, "2.5 dB", 1.25f},
    {ADC_6db, "6 dB", 1.75f},
    {ADC_11db, "11 dB", 3.10f},
};

constexpr size_t kAttenuationsCount = sizeof(kAttenuations) / sizeof(kAttenuations[0]);

float calcAdcVoltageFromRaw(uint16_t raw, float fsVoltage) {
    return (raw / (float)(ADC_MAX_CODE)) * fsVoltage;
}

uint16_t readRawAverage(uint8_t samples) {
    uint32_t sum = 0;

    for (uint8_t i = 0; i < samples; ++i) {
        sum += analogRead(ADC_PIN);
    }
    return sum / samples;
}

uint32_t readMilliVoltsAverage(uint8_t samples) {
    uint32_t sum = 0;

    for (uint8_t i = 0; i < samples; ++i) {
        sum += analogReadMilliVolts(ADC_PIN);
    }
    return sum / samples;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    analogReadResolution(12);
    pinMode(ADC_PIN, INPUT);

    Serial.println();
    Serial.println("=== ESP32-S3 ADC: divider 10k/1k, calculated vs compensated ===");
    Serial.printf("Divider ratio: %.3f\n", kDividerRatio);
    Serial.println("Atten | RAW | Vadc_calc | Vadc_comp");
}

void loop() {
    for (size_t i = 0; i < kAttenuationsCount; ++i) {
        const AttenuationConfig& cfg = kAttenuations[i];

        analogSetPinAttenuation(ADC_PIN, cfg.atten);
        delay(ATTENUATION_SETTLE_DELAY_MS);

        const uint16_t raw = readRawAverage(SAMPLES_PER_POINT);
        const uint32_t compmV = readMilliVoltsAverage(SAMPLES_PER_POINT);

        const float vadcCalc = calcAdcVoltageFromRaw(raw, cfg.fsVoltage);
        const float vadcComp = compmV / 1000.0f;

        Serial.printf(
            "%5s | %4u | %8.4fV | %8.4fV\n",
            cfg.name,
            raw,
            vadcCalc,
            vadcComp
        );
    }

    Serial.println("----------------------------------------------------------------");
    delay(LOOP_DELAY_MS);
}

