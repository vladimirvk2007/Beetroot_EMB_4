# Драйвери UART, I2C та OLED для ESP32-S3

Проєкт містить модулі UART та I2C для ESP32-S3 (ESP-IDF + PlatformIO), а також OLED-драйвер для SSD1306. Поточний приклад у `src/main.cpp` ініціалізує I2C і OLED 128x64, виводить заголовок та малює просту фігуру.

## Модуль I2C

- Драйвер: `lib/i2c/i2c.c` та `lib/i2c/i2c.h`
- Побудований на API `driver/i2c_master.h` з ESP-IDF; усі функції повертають `esp_err_t`.
- Шина: `i2c_bus_init()`, `i2c_bus_deinit()`, `i2c_bus_probe()`. Порт, піни SDA/SCL і внутрішня підтяжка задаються в `i2c_bus_settings_t`.
- Пристрій: `i2c_device_add()`, `i2c_device_remove()`. Адреса і швидкість SCL (0 = 100 кГц) задаються в `i2c_device_settings_t`.
- Обмін: `i2c_write()`, `i2c_read()`, `i2c_write_read()`, `i2c_write_register()`, `i2c_read_register()`.

## OLED-дисплей

- Драйвер: `lib/oled/oled.c` та `lib/oled/oled.h`
- Дисплей SSD1306 за адресою `0x3C`, шина I2C: SDA GPIO 8, SCL GPIO 9.
- Параметри дисплея задаються через `oled_settings_t`; `OLED_SETTINGS_128X64_DEFAULT()` встановлює типові параметри панелі 128x64, адресу `0x3C` і частоту I2C 400 кГц.
- API містить виведення тексту (`oled_print()`, `oled_printf()`, `oled_print_bdf()`), очищення (`oled_clear()`), керування контрастом і живленням, а також малювання пікселів, ліній, кіл та прямокутників, зокрема заповнених.
- Координати малювання задаються в пікселях від верхнього лівого кута: `(0, 0)` — початок екрана. Параметр `invert` інвертує намальовані пікселі; кожна функція малювання одразу оновлює дисплей.

Наприклад:

```c
ESP_ERROR_CHECK(oled_clear(oled));
ESP_ERROR_CHECK(oled_print(oled, 0, 1, false, "OLED drawing"));
ESP_ERROR_CHECK(oled_draw_line(oled, 10, 20, 100, 20, false));
ESP_ERROR_CHECK(oled_draw_rectangle(oled, 20, 28, 40, 24, false));
ESP_ERROR_CHECK(oled_fill_circle(oled, 90, 40, 8, false));
```

Підключіть SDA/SCL OLED до GPIO 8/GPIO 9, подайте живлення та з'єднайте GND. Шині потрібні підтягувальні резистори.

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
- Драйвер OLED: `lib/oled/oled.c`, `lib/oled/oled.h`
- Драйвер UART: `lib/uart/uart.c`, `lib/uart/uart.h`
- Конфігурація PlatformIO: `platformio.ini`
