#ifndef MQTT_H
#define MQTT_H

#include "mqtt_client.h"

#define MQTT_BROKER_URI  "mqtt://broker.hivemq.com:1883"
#define MQTT_TOPIC       "esp32s3/test"
#define MQTT_COMMANDS    "esp32s3/commands"
#define MQTT_STATUS      "esp32s3/status"

void mqtt_app_start(void);
esp_mqtt_client_handle_t mqtt_get_client(void);

#endif // MQTT_H