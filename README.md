# ESP32-S3 UART and GPIO Example

This project uses ESP-IDF in PlatformIO. It polls a button on GPIO 15, controls an active-low LED on GPIO 16, and echoes bytes received over UART1.

## UART

- Driver: `src/uart/uart.c` and `src/uart/uart.h`
- UART1: 115200 baud, 8N1
- TX: GPIO 17
- RX: GPIO 18
- Connect the USB-UART adapter TX to GPIO 18, RX to GPIO 17, and connect GND.
- The USB serial console remains separate for application logs.

## PlatformIO Configuration
- Platform: `espressif32`
- Board: `esp32-s3-devkitc-1`
- Framework: `espidf`
- Monitor speed: 115200 baud
- Upload: auto-detect port

## How to Build and Flash
1. Connect your ESP32 board to the computer.
2. Open a terminal in the project root.
3. Run:
   ```
   pio run -t upload
   ```
4. To view logs, use:
   ```
   pio device monitor
   ```

## Main Code

- Application: `src/main.cpp`
- UART echo runs alongside button polling and LED control.
