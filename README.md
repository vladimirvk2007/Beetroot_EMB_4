
# STM32F4 STM32CubeMX Compatible Example

This project demonstrates a simple application for the STM32F411 Black Pill board: blinking LEDs and sending messages over USB (CDC virtual COM port).

## Description
- Blinks a single LED connected to PC14 every 500 ms.
- Sends the string "LED blinked!" via USB CDC (virtual COM port) every 500 ms.

## STM32CubeMX Compatibility
This version of the program is fully compatible with STM32CubeMX configuration (tested with STM32CubeMX Version 6.16.0). You can open the `src/src.ioc` file in STM32CubeMX (open it via your file browser), make additional hardware or middleware settings, and regenerate the code as needed. All CubeMX-generated files and user code sections are preserved for easy further customization.

## Project Structure
- `src/Core/Src/main.c` — main application file
- `src/Core/Inc/` — header files
- `USB_DEVICE/` — USB (CDC) middleware files
- `platformio.ini` — PlatformIO configuration
- `src/src.ioc` — STM32CubeMX project file for configuration


## Build and Flash Instructions

Programming is performed using an ST-Link programmer/debugger (default for PlatformIO STM32 projects).

You can use PlatformIO CLI to build and upload the firmware:

```
# Build the project
pio run

# Build and upload (flash) to the board
pio run -t upload

# Optional: open serial monitor
pio device monitor
```

## Requirements
- STM32F411 Black Pill board
- One LED connected to PC14
- USB cable for PC connection

## Usage
1. Connect the board to your PC via USB.
2. After flashing:
   - The LED on PC14 will blink every 500 ms.
   - The virtual COM port will send the message "LED blinked!" every 500 ms.
