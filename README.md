# ESP32-S3 UART Demonstration

This project demonstrates UART communication on the ESP32-S3 using ESP-IDF and PlatformIO. The application receives bytes over UART1, logs each received byte in HEX and character formats, and echoes it back to the sender.

## UART Configuration

- Driver: `lib/uart/uart.c` and `lib/uart/uart.h`
- Interface: UART1, 115200 baud, 8 data bits, no parity, 1 stop bit (8N1)
- TX: GPIO 17
- RX: GPIO 18
- `uart_receive()` and `uart_transmit()` return `esp_err_t`. Receive timeout is reported as `ESP_ERR_TIMEOUT`.

Connect the USB-UART adapter TX to GPIO 18, RX to GPIO 17, and connect GND. The USB serial console is separate and is used for application logs.

## Build and Flash

- Platform: `espressif32`
- Board: `esp32-s3-devkitc-1`
- Framework: `espidf`

Build and upload with:

```sh
pio run -t upload
```

View application logs with:

```sh
pio run -t monitor
```

## Source Files

- Application: `src/main.cpp`
- UART driver: `lib/uart/uart.c` and `lib/uart/uart.h`
