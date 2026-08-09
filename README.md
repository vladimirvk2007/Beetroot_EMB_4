# ESP32 LED/Button Mode Demo

A small PlatformIO project for an ESP32-S3 board that demonstrates basic GPIO control with two LEDs and two buttons. The firmware switches between three blinking modes depending on which button is pressed.

## Overview

This project is a simple embedded demo for learning:
- button input handling
- simple debouncing
- LED control
- basic state-based behavior in Arduino firmware

## Hardware

- Board: ESP32-S3 DevKitC-1
- LEDs:
  - Red LED on GPIO 15
  - Green LED on GPIO 16
- Buttons:
  - External button on GPIO 17
  - BOOT button on GPIO 0

## Functionality

The firmware supports three modes:
- Slow mode: default blinking delay of 500 ms
- Fast mode: activated by pressing the external button
- Extra mode: activated by pressing both buttons simultaneously

## Project Files

- [src/main.cpp](src/main.cpp) — main firmware logic
- [platformio.ini](platformio.ini) — PlatformIO configuration

## Build and Upload

From the project root, run:

```bash
pio run
pio run -t upload
```

## Notes

The serial monitor is configured for 115200 baud rate in [platformio.ini](platformio.ini).


