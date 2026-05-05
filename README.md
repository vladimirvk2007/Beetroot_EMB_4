# ESP32-S3 MQTT Demo

Демонстраційний проект для платформи **ESP32-S3**, який реалізує публікацію та прийом MQTT-повідомлень через публічний брокер HiveMQ.

Фреймворк: **ESP-IDF** (native, без Arduino).

## Можливості

- Підключення до Wi-Fi та автоматичне перепідключення
- Публікація повідомлень кожні 10 секунд із лічильником
- Прийом команд через MQTT та керування GPIO (LED)
- Відповідь на запит статусу пристрою

## Апаратне забезпечення

| Компонент | Специфікація |
|-----------|-------------|
| Плата | ESP32-S3-DevKitC-1 |
| Flash | 16 MB (QIO) |
| PSRAM | 8 MB (OPI) |
| LED | GPIO 16 |

## MQTT топіки

| Топік | Напрямок | Опис |
|-------|----------|------|
| `esp32s3/test` | Публікація | Повідомлення `Hello from ESP32-S3! #N` кожні 10 с |
| `esp32s3/commands` | Підписка | Вхідні команди керування |
| `esp32s3/status` | Публікація | Відповідь на команду `STATUS` |

### Підтримувані команди

| Команда | Дія |
|---------|-----|
| `ON` | Увімкнути LED (GPIO 16) |
| `OFF` | Вимкнути LED (GPIO 16) |
| `STATUS` | Надіслати статус на `esp32s3/status` |

## Налаштування

### Wi-Fi та MQTT брокер

Облікові дані Wi-Fi зберігаються у `lib/credentials/credentials.h`:

```c
#define WIFI_SSID     "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"
```

Налаштування брокера у `lib/mqtt/mqtt.h`:

```c
#define MQTT_BROKER_URI  "mqtt://broker.hivemq.com:1883"
```

## Збірка та прошивка

```bash
# Збірка
pio run

# Прошивка та моніторинг
pio run -t upload -t monitor
```

## Перегляд MQTT-повідомлень

**MQTT Explorer** (рекомендовано):
1. Завантажити: https://mqtt-explorer.com/
2. Підключитись до `broker.hivemq.com:1883`
3. Підписатись на топік `esp32s3/#`

**[Веб-клієнт HiveMQ](https://www.hivemq.com/demos/websocket-client/)** (без встановлення, прямо в браузері):

**Підключення:**
1. Відкрити [https://www.hivemq.com/demos/websocket-client/](https://www.hivemq.com/demos/websocket-client/)
2. Натиснути **Connect** — статус зміниться на `Connected`

**Підписка на повідомлення з ESP32-S3:**
1. У секції **Subscriptions** натиснути **Add New Topic Subscription**
2. Вписати топік `esp32s3/#` (усі повідомлення від пристрою) або `esp32s3/test`
3. Натиснути **Subscribe**
4. Нові повідомлення з'являтимуться у секції **Messages**

**Надсилання команд на ESP32-S3:**
1. У секції **Publish** вказати:
   - Topic: `esp32s3/commands`
   - Message: `ON`, `OFF` або `STATUS`
2. Натиснути **Publish**
3. Відповідь на `STATUS` з'явиться у топіку `esp32s3/status`

**Командний рядок**:
```bash
mosquitto_sub -h broker.hivemq.com -t "esp32s3/#"
```

## Залежності

- **esp-mqtt** — вбудований компонент ESP-IDF, окремого встановлення не потребує
- **esp-wifi** — вбудований компонент ESP-IDF
- Framework: ESP-IDF
- Platform: espressif32
