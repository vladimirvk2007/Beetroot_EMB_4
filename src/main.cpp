#include <Arduino.h>

#define ADC_PIN             4
#define LED_PIN             15

#define VREF                3.0
#define ADC_RESOLUTION      4095.0
#define VOLTAGE_THRESHOLD   1.5
#define VOLTAGE_GIST        0.2


float getVoltage(int adcValue) {
    float voltage = 0.0;

    voltage = (adcValue / ADC_RESOLUTION) * VREF;

    return voltage;
}


void setup() {
    // Start the Serial Monitor at 115200 baud
    Serial.begin(115200);
    analogReadResolution(12); // Set the ADC resolution to 12 bits (0-4095)
    pinMode(LED_PIN, OUTPUT); // Set the LED pin as an output

    Serial.println("Everything is set up and ready to go!");

}

void loop() {
    int adcValue = 0;
    float voltage = 0.0;

    adcValue = analogRead(ADC_PIN); // Read the ADC value from the specified pin
    voltage = getVoltage(adcValue); // Convert the ADC value to voltage

    Serial.printf("ADC value %d, Voltage %.2f V\n", adcValue, voltage); // Print the ADC value and voltage to the Serial Monitor

    if (voltage < VOLTAGE_THRESHOLD - VOLTAGE_GIST) {
        digitalWrite(LED_PIN, LOW); // Turn on the LED if voltage is below threshold
    } else if (voltage > VOLTAGE_THRESHOLD + VOLTAGE_GIST) {
        digitalWrite(LED_PIN, HIGH); // Turn off the LED if voltage is above threshold
    }

    delay(200); // Wait for 200ms before the next reading
}

