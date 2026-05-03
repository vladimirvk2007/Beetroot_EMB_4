# ESP32-S3 MQTT Demo

Демонстраційний проект для платформи **ESP32-S3**, який реалізує публікацію та прийом MQTT-повідомлень через публічний брокер HiveMQ.

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
#define MQTT_SERVER  "broker.hivemq.com"
#define MQTT_PORT    1883
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

**Веб-клієнт HiveMQ**:
1. Відкрити https://www.hivemq.com/demos/websocket-client/
2. Host: `broker.hivemq.com`, Port: `8000`
3. Підписатись на `esp32s3/test`

**Командний рядок**:
```bash
mosquitto_sub -h broker.hivemq.com -t "esp32s3/#"
```

## Залежності

- [knolleary/PubSubClient](https://github.com/knolleary/pubsubclient) — MQTT клієнт
- Framework: Arduino (ESP-IDF)
- Platform: espressif32
