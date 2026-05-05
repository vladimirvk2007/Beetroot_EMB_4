#include "mqtt.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <string.h>

static const char *TAG = "mqtt";
static esp_mqtt_client_handle_t s_client = NULL;

#define LED_GPIO GPIO_NUM_16

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            esp_mqtt_client_subscribe(client, MQTT_COMMANDS, 0);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT disconnected");
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Topic: %.*s, Data: %.*s",
                     event->topic_len, event->topic,
                     event->data_len, event->data);

            if (strncmp(event->topic, MQTT_COMMANDS, event->topic_len) == 0) {
                if (strncmp(event->data, "ON", event->data_len) == 0) {
                    ESP_LOGI(TAG, "Command: LED ON");
                    gpio_set_level(LED_GPIO, 1);
                } else if (strncmp(event->data, "OFF", event->data_len) == 0) {
                    ESP_LOGI(TAG, "Command: LED OFF");
                    gpio_set_level(LED_GPIO, 0);
                } else if (strncmp(event->data, "STATUS", event->data_len) == 0) {
                    esp_mqtt_client_publish(client, MQTT_STATUS, "ESP32-S3 is running", 0, 0, 0);
                    ESP_LOGI(TAG, "Status sent");
                } else {
                    ESP_LOGW(TAG, "Unknown command");
                }
            }
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error");
            break;

        default:
            break;
    }
}

void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = MQTT_BROKER_URI;

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(s_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
    esp_mqtt_client_start(s_client);
}

esp_mqtt_client_handle_t mqtt_get_client(void)
{
    return s_client;
}