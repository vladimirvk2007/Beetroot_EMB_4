# STM32F411 Demo Project

This repository contains a demo project for the STM32F411 Black Pill board (STM32F411CEU6), created with STM32CubeIDE. The project demonstrates basic firmware structure and is ready for further development and customization.

## Features
- STM32F411CEU6 microcontroller support
- STM32CubeIDE project structure
- HAL drivers and CMSIS included
- USB Device middleware (CDC example)
- Configured virtual COM port (CDC) via USB Type-C for log transmission
- On-board indicator (LED) connected to PC13 blinks periodically
- Ready-to-use linker scripts and Makefile

## Project Structure
```
STM32F411_demo/
├── Core/
│   ├── Inc/         # Header files
│   └── Src/         # Source files
├── Drivers/         # HAL and CMSIS drivers
├── Middlewares/     # USB Device libraries
├── USB_DEVICE/      # USB device application and target
├── Startup/         # Startup assembly code
├── *.ld             # Linker scripts
├── *.ioc            # STM32CubeMX configuration file
├── makefile         # Makefile for building (optional)
└── .gitignore       # Git ignore rules
```

## Getting Started
1. **Clone the repository:**
   ```sh
   git clone <repo-url>
   ```
2. **Import the project into STM32CubeIDE:**
   - Open STM32CubeIDE
   - File → Import... → Existing Projects into Workspace
   - Select the root folder of this repository
   - Click Finish
3. **Open and build via *.ioc file:**
   - Open the *.ioc file by double-clicking it in the Project Explorer
   - Make any necessary adjustments (if needed)
   - Save changes (Ctrl+S)
   - Build the project (Project → Build All or the Build button)
    - Connect your STM32F411 board.
    - Flash the board using ST-Link (required for programming):
       - ST-Link connection pins on STM32F411:
          - SWDIO → SWDIO
          - SWCLK → SWCLK
          - GND   → GND
          - 3.3V  → 3.3V (ST-Link can power the board via this pin)

## On-board Indicator (LED)
The board has an indicator LED connected to pin PC13. The firmware blinks this LED periodically to indicate operation. No external components are required—just use the on-board LED (PC13).

## Requirements
- STM32CubeIDE (Version: 1.19.0 recommended)
- STM32F411 Black Pill development board (STM32F411CEU6)

## Notes
- The `.ioc` file contains the CubeMX configuration. You can modify it and regenerate code as needed.
- All source code and configuration files required for building are included.
- Build artifacts and user-specific files are excluded via `.gitignore`.

