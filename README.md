
# ESP32 Hardware Timer Example with Button

This project demonstrates the use of a hardware timer on the ESP32 platform, with control via a single button.

## Features
- Hardware timer triggers an interrupt every 1 second (1,000,000 microseconds)
- Each timer interrupt increments an atomic counter and sets a flag
- Main loop prints the trigger count and timestamp to the serial monitor
- A button (connected to pin 15) stops and deletes the timer when pressed

## Pinout
- **Button:** Connect to pin 15 (GPIO15) with a pull-up resistor.
 - **UART-USB Adapter:** Connect RX of the adapter to PA8 (TX) pin on the board

## How it works
1. On startup, the timer is started automatically
2. Each timer event prints a message to the serial monitor
3. Press the button to stop and delete the timer at any time. The timer cannot be restarted without resetting the board.

## Requirements
- ESP32 board (e.g., ESP32-S3 DevKit)
- PlatformIO
- A button with a pull-up resistor

