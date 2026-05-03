#include <WiFi.h>
#include <PubSubClient.h>
#include "credentials.h"
#include "mqtt.h"
#include "wifi_setup.h"

#define UART_BAUD_RATE 115200
#define LED_OUT   16

#define PUBLISH_INTERVAL_MS (10 * 1000)

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

    // Обробка команд з топіку MQTT_COMMANDS
    if (String(topic) == MQTT_COMMANDS) {
        if (message == "ON") {
            Serial.println("Command: LED ON");
            digitalWrite(LED_OUT, HIGH);
        } else if (message == "OFF") {
            Serial.println("Command: LED OFF");
            digitalWrite(LED_OUT, LOW);
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
    delay(100);
    setup_wifi();
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(callback);
    pinMode(LED_OUT, OUTPUT);

}

void loop() {
    if (!client.connected()) {
        mqtt_reconnect(&client);
    }
    client.loop();

    unsigned long now = millis();
    if (now - lastMsg > PUBLISH_INTERVAL_MS) {
        lastMsg = now;
        msgCount++;
        char msg[64];
        snprintf(msg, sizeof(msg), "Hello from ESP32-S3! #%lu", msgCount);
        client.publish(MQTT_TOPIC, msg);
        Serial.print("Published: ");
        Serial.println(msg);
    }
}
