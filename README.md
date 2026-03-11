# STM32F411 Black Pill Startup

This project is a simple startup example for the STM32F411 Black Pill board using PlatformIO.

## Features
- Basic setup for STM32F411 development
- UART output is configured on pin A9
- Indicator (LED) connected to pin PC13

## Getting Started
1. Clone this repository.
2. Open the project with PlatformIO in VS Code.
3. Build and upload the firmware to your STM32F411 Black Pill board.

## UART Output
- UART TX is connected to pin **A9**.

## Indicator (LED)
- The indicator LED is connected to pin **PC13**.

## Requirements
- PlatformIO
- STM32F411 Black Pill development board

## Programming
To upload the firmware, connect an ST-Link programmer to the following SWD pins on the STM32F411 Black Pill board:

| ST-Link Pin | STM32F411 Pin |
|-------------|---------------|
| SWDIO       | SWDIO         |
| SWCLK       | SWCLK         |
| GND         | GND           |
| 3.3V        | 3.3V          |

Make sure to match each pin correctly for successful programming.

Note: The ST-Link provides power to the STM32F411 Black Pill board through the 3.3V and GND pins, so no additional power supply is required during programming.

