#include "mqtt.h"

#define MQTT_CLIENT_ID_STR "ESP32S3Client-"

void mqtt_reconnect(PubSubClient *client) {
    if (client == nullptr) {
        return;
    }

    while (!client->connected()) {
        Serial.print("Attempting MQTT connection...");
        // Створюємо унікальний ID клієнта
        String clientId = MQTT_CLIENT_ID_STR;
        clientId += String(random(0xffff), HEX);

        if (client->connect(clientId.c_str())) {
            Serial.println("connected");
            client->subscribe(MQTT_COMMANDS); // Підписка на вхідні команди
        } else {
            Serial.print("failed, rc=");
            Serial.print(client->state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}