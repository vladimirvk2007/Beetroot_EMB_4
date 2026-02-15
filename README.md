# ESP32 DS18B20 Temperature Reader

Temperature monitoring system using ESP32 and DS18B20 one-wire temperature sensor.

[DS18B20 Datasheet](https://cdn.sparkfun.com/datasheets/Sensors/Temp/DS18B20.pdf)

## Overview

This PlatformIO project reads temperature data from a DS18B20 one-wire temperature sensor using an ESP32 board. The program requests temperature readings every 2 seconds and displays them on the serial monitor.

## Hardware Requirements

- **Microcontroller:** ESP32 or ESP32-S3 (tested on YD-ESP32-S3)
- **Temperature Sensor:** DS18B20 one-wire sensor
- **Pull-up Resistor:** 4.7 kΩ (between Data pin and 3.3V)
- **Power Supply:** 3.3V from ESP32

## Wiring Diagram

| DS18B20 Pin | ESP32 Pin | Notes |
|------------|-----------|-------|
| VCC (Pin 1) | 3.3V | Power supply |
| Data (Pin 2) | GPIO4 | One-wire data line (with 4.7kΩ pull-up) |
| GND (Pin 3) | GND | Ground |

**Important:** The 4.7 kΩ pull-up resistor between Data pin and 3.3V is required for reliable bus operation.

## Software & Libraries

- **Framework:** Arduino framework for ESP32
- **Build Tool:** PlatformIO
- **Required Libraries:**
  - OneWire
  - DallasTemperature

## Build & Upload

### Using PlatformIO CLI

```bash
# Build project
pio run

# Upload firmware to ESP32
pio run -t upload

# Monitor serial output at 115200 baud
pio device monitor -b 115200

# Combine upload and monitor
pio run -t upload && pio device monitor -b 115200
```

### Using PlatformIO in VS Code
1. Open the project folder in VS Code
2. Click **Build** (✓ icon) in the bottom toolbar
3. Click **Upload** (→ icon) to flash the ESP32
4. Click **Serial Monitor** to view output

## Usage

1. **Wire the hardware** as shown in the Wiring Diagram
2. **Build and upload** the firmware using PlatformIO
3. **Open serial monitor** at 115200 baud to view temperature readings
4. Temperature readings display every 2 seconds

## Expected Output

```
DS18B20 Temperature Sensor Test
Found 1 sensor(s).
Requesting temperatures...DONE
Temperature for Device 1: 24.50 °C
Temperature for Device 1: 24.56 °C
Temperature for Device 1: 24.62 °C
...
```

## Configuration

To change the GPIO pin used for the sensor, edit `src/main.cpp`:

```cpp
#define ONE_WIRE_BUS 4  // Change this to your desired GPIO pin
```

## Troubleshooting

| Issue | Solution |
|-------|----------|
| No sensor detected | 1. Check wiring<br>2. Verify 4.7kΩ pull-up resistor is present<br>3. Confirm correct GPIO pin in code |
| Garbled serial output | 1. Set monitor speed to 115200 baud<br>2. Check USB cable connection<br>3. Try different USB port |
| Intermittent readings | 1. Check wire connections for loose contacts<br>2. Verify pull-up resistor value (should be 4.7kΩ)<br>3. Try shorter wires (reduce noise) |
| Wrong temperature values | 1. Verify sensor is genuine DS18B20<br>2. Check power supply voltage (should be 3.3V or 5V)<br>3. Sensor may need warm-up time (~750ms) |

## Performance Notes

- **Sensor Resolution:** 0.5°C increments (by default)
- **Conversion Time:** ~750 ms per reading
- **Accuracy:** ±0.5°C typical
- **Temperature Range:** -55°C to +125°C
- **Multiple Sensors:** This library supports multiple DS18B20 sensors on the same bus (each has unique ROM address)

## References

- [DS18B20 Datasheet](https://cdn.sparkfun.com/datasheets/Sensors/Temp/DS18B20.pdf)
- [OneWire Library](https://github.com/PaulStoffregen/OneWire/blob/master/OneWire.h)
- [DallasTemperature Library](https://github.com/milesburton/Arduino-Temperature-Control-Library/blob/master/DallasTemperature.h)

## License

Open source. Use and modify freely.
