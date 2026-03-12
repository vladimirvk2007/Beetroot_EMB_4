


# ESP32 Timer with Button Control


This project demonstrates the use of a hardware timer on the ESP32 platform. The timer triggers an interrupt every 1 second, increments an atomic counter, and prints the count and timestamp to the serial monitor.

Button handling is implemented using the [OneButton](https://github.com/mathertel/OneButton) library for reliable and debounced button presses.

A button connected to pin 15 (GPIO15) is used to stop and restart the timer when pressed. Each short press toggles the timer state: if the timer is running, it will be stopped and deleted; if the timer is stopped, it will be created and started again.

## Features
- Hardware timer interrupt every 1 second
- Atomic counter for safe ISR/main loop communication
- Serial output of trigger count and time
- Button press toggles (stops and restarts) the timer

## Pinout
- **Button:** Connect to pin 15 (GPIO15), GND, and Vcc (pull-up connection)
- **UART-USB Adapter:** Connect RX of the adapter to PA8 (TX) pin on the board

## Requirements
- ESP32 board (e.g., ESP32-S3 DevKit)
- PlatformIO
- Standard push button
- OneButton library (already included in PlatformIO dependencies)

## Usage
1. Build and upload the firmware using PlatformIO.
2. Open the serial monitor at 115200 baud.
3. Press the button to stop the timer. Press again to restart the timer. You can toggle the timer as many times as you want.

