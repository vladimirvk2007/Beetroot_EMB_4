# ESP32 LED Blinking Example

This project demonstrates a simple LED blinking application for ESP32 using ESP-IDF in PlatformIO. An LED connected to GPIO 16 blinks with a 500 ms interval. Each time the LED changes state, a message ("LED ON" or "LED OFF") is printed to the serial console.

## Main Code
- File: `src/main.c`
- GPIO: 16 (can be changed in the `BLINK_GPIO` macro)
- Logging: via `printf` to the serial console

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

## Additional Notes
- To change the GPIO, modify the `BLINK_GPIO` macro in `main.c`.
- To change the blink frequency, adjust the delay in the `vTaskDelay` function.

