#ifndef MQTT_H
#define MQTT_H

#include <PubSubClient.h>

#define MQTT_SERVER     "broker.hivemq.com"
#define MQTT_PORT       1883
#define MQTT_TOPIC      "esp32s3/test"
#define MQTT_COMMANDS   "esp32s3/commands"
#define MQTT_STATUS     "esp32s3/status"

void mqtt_reconnect(PubSubClient *client);

#endif // MQTT_H