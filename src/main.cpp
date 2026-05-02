#include <WiFi.h>
#include <PubSubClient.h>
#include "credentials.h"
#include "mqtt.h"
#include "wifi_setup.h"

#define UART_BAUD_RATE 115200

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
unsigned long msgCount = 0;

// Функція обробки вхідних повідомлень
void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.println(message);

    // Обробка команд з топіку esp32s3/commands
    if (String(topic) == MQTT_COMMANDS) {
        if (message == "ON") {
            Serial.println("Command: LED ON");
            // digitalWrite(LED_PIN, HIGH);
        } else if (message == "OFF") {
            Serial.println("Command: LED OFF");
            // digitalWrite(LED_PIN, LOW);
        } else if (message == "STATUS") {
            client.publish(MQTT_STATUS, "ESP32-S3 is running");
            Serial.println("Status sent");
        } else {
            Serial.println("Unknown command");
        }
    }
}

void setup() {
    Serial.begin(UART_BAUD_RATE);
    delay(100);  // Затримка для ініціалізації Serial на ESP32-S3
    setup_wifi();
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(callback);
}

void loop() {
    if (!client.connected()) {
        mqtt_reconnect(&client);
    }
    client.loop();

    unsigned long now = millis();
    if (now - lastMsg > 10000) {
        lastMsg = now;
        msgCount++;
        char msg[64];
        snprintf(msg, sizeof(msg), "Hello from ESP32-S3! #%lu", msgCount);
        client.publish(MQTT_TOPIC, msg);
        Serial.print("Published: ");
        Serial.println(msg);
    }
}
