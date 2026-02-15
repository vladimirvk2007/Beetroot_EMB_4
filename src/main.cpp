#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into GPIO 4 on the YD-ESP32-S3
// Remember to use a 4.7k Ohm pull-up resistor between Data and 3.3V
const int ONE_WIRE_BUS = 4;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature library
DallasTemperature sensors(&oneWire);

void setup() {
  // Start the Serial Monitor at 115200 baud
  Serial.begin(115200);
  Serial.println("DS18B20 Temperature Sensor Test");

  // Start up the library
  sensors.begin();

  // Check if at least one sensor is connected
  if (sensors.getDeviceCount() == 0) {
    Serial.println("Error: No DS18B20 sensor found! Please check wiring and pull-up resistor.");
  } else {
    Serial.print("Found ");
    Serial.print(sensors.getDeviceCount());
    Serial.println(" sensor(s).");
  }
}

void loop() {
  // Send the command to all sensors on the bus to get temperatures
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures();
  Serial.println("DONE");

  // Fetch temperature in Celsius for the first sensor (index 0)
  float tempC = sensors.getTempCByIndex(0);

  // Check if reading was successful
  if (tempC != DEVICE_DISCONNECTED_C) {
    Serial.print("Temperature for Device 1: ");
    Serial.print(tempC);
    Serial.println(" °C");

    // Optional: Convert to Fahrenheit
    // float tempF = sensors.toFahrenheit(tempC);
    // Serial.print(tempF);
    // Serial.println(" °F");
  } else {
    Serial.println("Error: Could not read temperature data");
  }

  // Wait 2 seconds before the next reading
  // Also print memory info periodically

  delay(2000);
}

