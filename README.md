# ESP32 Light Sensor Control

A simple PlatformIO project for ESP32-S3 that reads a light sensor (photoresistor in voltage divider circuit) and controls an LED based on light intensity.

## Hardware

**Circuit**: +3.3V → Photoresistor → GPIO4 (ADC) → 10kΩ Resistor → GND

| Component | Pin |
|-----------|-----|
| LDR + 10kΩ divider | GPIO4 (ADC) |
| LED control | GPIO15 |

**Board**: ESP32-S3-DevKitC-1 (16MB Flash, 8MB PSRAM)

## Operation

- Reads ADC voltage every 200ms
- LED control: Voltage < 1.3V (ON) | 1.3V-1.7V (no change) | > 1.7V (OFF)
- Serial output at 115200 baud: ADC value and voltage

**Configurable in `src/main.cpp`**:
- `ADC_PIN`: 4
- `LED_PIN`: 15
- `VOLTAGE_THRESHOLD`: 1.5V
- `VOLTAGE_GIST`: 0.2V (hysteresis)

## Building & Running

```bash
pio run              # Build
pio run -t upload    # Upload to board
pio device monitor --baud 115200  # Monitor output
```

Or all at once:
```bash
pio run -t upload -t monitor
```
