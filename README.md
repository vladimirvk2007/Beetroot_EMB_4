# Драйвери UART та I2C для ESP32-S3

Проєкт містить невеликі універсальні модулі UART та I2C для ESP32-S3 (ESP-IDF + PlatformIO). Приклад у `src/main.cpp` демонструє роботу I2C: читання регістрів RTC DS1307.

## Модуль I2C

- Драйвер: `lib/i2c/i2c.c` та `lib/i2c/i2c.h`
- Побудований на API `driver/i2c_master.h` з ESP-IDF; усі функції повертають `esp_err_t`.
- Шина: `i2c_bus_init()`, `i2c_bus_deinit()`, `i2c_bus_probe()`. Порт, піни SDA/SCL і внутрішня підтяжка задаються в `i2c_bus_settings_t`.
- Пристрій: `i2c_device_add()`, `i2c_device_remove()`. Адреса і швидкість SCL (0 = 100 кГц) задаються в `i2c_device_settings_t`.
- Обмін: `i2c_write()`, `i2c_read()`, `i2c_write_read()`, `i2c_write_register()`, `i2c_read_register()`.

## Приклад

`src/main.cpp` ініціалізує I2C порт 0 (SDA GPIO 8, SCL GPIO 9), перевіряє наявність DS1307 за адресою 0x68 і раз на секунду читає регістри 0x00-0x07, виводячи їх у лог у HEX.

Підключіть SDA/SCL модуля DS1307 до GPIO 8/GPIO 9, подайте живлення та з'єднайте GND. Шині потрібні підтягувальні резистори.

## Модуль UART

- Драйвер: `lib/uart/uart.c` та `lib/uart/uart.h`
- Інтерфейс: UART1, 115200 бод, 8N1, TX GPIO 17, RX GPIO 18
- `uart_init()`, `uart_receive()` та `uart_transmit()` повертають `esp_err_t`. Таймаут прийому повертається як `ESP_ERR_TIMEOUT`.

Поточний приклад модуль UART не використовує.

## Збірка та прошивка

- Платформа: `espressif32`
- Плата: YD-ESP32-S3 (`esp32-s3-devkitc-1` у PlatformIO)
- Фреймворк: `espidf`

```sh
pio run -t upload
pio run -t monitor
```

## Файли

- Застосунок: `src/main.cpp`
- Драйвер I2C: `lib/i2c/i2c.c`, `lib/i2c/i2c.h`
- Драйвер UART: `lib/uart/uart.c`, `lib/uart/uart.h`
