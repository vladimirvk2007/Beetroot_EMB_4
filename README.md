

# STM32F4 STM32CubeMX Compatible Example (C++ version)

This project demonstrates a simple application for the STM32F411 Black Pill board: blinking an LED and sending messages over USB (CDC virtual COM port), using a C++ class for LED control.

## Description
- Blinks one LED (PC13) every 500 ms.
- Sends the string "LED blinked!" via USB CDC (virtual COM port) every 500 ms.
- Uses the Led class for convenient LED control (see src/Core/Src/main_app.cpp).

## C++ Support
The project uses main_app.cpp with the Led class to demonstrate an object-oriented approach to GPIO control. You can extend the functionality by adding your own C++ classes and methods.

## STM32CubeMX Compatibility
This version is fully compatible with STM32CubeMX (tested with version 6.16.0). You can open the `src/src.ioc` file in STM32CubeMX, change peripheral settings, and regenerate the code. All user code sections are preserved for further extension.

## Project Structure
- `src/Core/Src/main_app.cpp` — main C++ file with the Led class
- `src/Core/Src/main.c` — entry point (C, calls main_cpp)
- `src/Core/Inc/` — header files
- `USB_DEVICE/` — USB (CDC) middleware files
- `platformio.ini` — PlatformIO configuration
- `src/src.ioc` — STM32CubeMX project file for configuration

## Build and Flash Instructions

Programming is performed using an ST-Link programmer/debugger (default for PlatformIO STM32 projects).

You can use the PlatformIO CLI to build and upload the firmware:

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
- One LED connected to PC13
- USB cable for PC connection

## Usage
1. Connect the board to your PC via USB.
2. After flashing:
   - The LED on PC13 will blink every 500 ms.
   - The virtual COM port will send the message "LED blinked!" every 500 ms.
