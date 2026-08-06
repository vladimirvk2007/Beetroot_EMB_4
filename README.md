# Hello World

Простий firmware-проєкт Hello World на Arduino framework для ESP32-S3, який перемикає стан світлодіода раз на 1 секунду та виводить статус у Serial Monitor.

## Що робить програма

- Ініціалізує Serial зі швидкістю `115200`.
- Налаштовує GPIO `15` як вихід (`LED_OUT`).
- Кожну секунду перемикає стан LED:
	- `LED ON`
	- `LED OFF`

Основна логіка знаходиться у `src/main.cpp`.

## Конфігурація плати (PlatformIO)

Проєкт налаштований для середовища:

- `board`: `esp32-s3-devkitc-1`
- `platform`: `espressif32`
- `framework`: `arduino`
- `monitor_speed`: `115200`

Примітка: фізична плата в цьому проєкті - YD-ESP32-S3 (ESP32-S3-N16R8), а в PlatformIO для неї використовується профіль `esp32-s3-devkitc-1`.

Додатково ввімкнені параметри для 16MB flash та PSRAM (OPI) у `platformio.ini`.

## Вимоги

- VS Code
- Розширення PlatformIO IDE
- Плата `YD-ESP32-S3 (ESP32-S3-N16R8)`

## Збірка

```bash
pio run
```

## Прошивка

```bash
pio run -t upload
```

## Перегляд логів (Serial Monitor)

```bash
pio run -t monitor
```

У моніторі ви повинні бачити почергові повідомлення `LED ON` / `LED OFF`.
