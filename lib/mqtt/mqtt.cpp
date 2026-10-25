#include "mqtt.h"
#include "esp_log.h"
#include <string.h>

#define BUFFER_SIZE 128

static const char *TAG = "mqtt";
static esp_mqtt_client_handle_t s_client = NULL;
static mqtt_message_handler_t s_message_handler = NULL;
static volatile bool s_connected = false;

// Виводить доступні відомості про транспортну помилку або відмову брокера.
static void log_mqtt_error(const esp_mqtt_error_codes_t *error)
{
    if (error == NULL) {
        ESP_LOGE(TAG, "MQTT error event has no error details");
        return;
    }

    ESP_LOGE(TAG, "MQTT error type: %d", error->error_type);
    if (error->esp_tls_last_esp_err != ESP_OK) {
        ESP_LOGE(TAG, "TLS error: %s (0x%x)",
                 esp_err_to_name(error->esp_tls_last_esp_err),
                 (unsigned int)error->esp_tls_last_esp_err);
    }
    if (error->esp_tls_stack_err != 0) {
        ESP_LOGE(TAG, "TLS stack error: %d", error->esp_tls_stack_err);
    }
    if (error->esp_tls_cert_verify_flags != 0) {
        ESP_LOGE(TAG, "TLS certificate verification flags: 0x%x",
                 (unsigned int)error->esp_tls_cert_verify_flags);
    }
    if (error->esp_transport_sock_errno != 0) {
        ESP_LOGE(TAG, "Socket error: %s (%d)",
                 strerror(error->esp_transport_sock_errno),
                 error->esp_transport_sock_errno);
    }
    if (error->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
        ESP_LOGE(TAG, "Broker refused MQTT connection (code=%d)",
                 error->connect_return_code);
    }
}

static void destroy_mqtt_client(void)
{
    // Прибираємо глобальний handle, щоб повторний запуск не використовував знищений клієнт.
    esp_err_t err = esp_mqtt_client_destroy(s_client);
    s_client = NULL;
    s_connected = false;
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to destroy MQTT client: %s (0x%x)",
                 esp_err_to_name(err), (unsigned int)err);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    if (event_data == NULL) {
        ESP_LOGW(TAG, "Ignoring MQTT event without data (event=%d)", (int)event_id);
        return;
    }

    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;
    char topic_buf[BUFFER_SIZE] = {0};
    char data_buf[BUFFER_SIZE] = {0};
    int topic_len = 0;
    int data_len = 0;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            s_connected = true;
            {
                // Підписка виконується при кожному з'єднанні, зокрема після перепідключення.
                int msg_id = esp_mqtt_client_subscribe(client, MQTT_COMMANDS, 0);
                if (msg_id < 0) {
                    ESP_LOGE(TAG, "Failed to subscribe to %s (error=%d)",
                             MQTT_COMMANDS, msg_id);
                }
            }
            break;

        case MQTT_EVENT_DISCONNECTED:
            s_connected = false;
            ESP_LOGW(TAG, "MQTT disconnected; the client will attempt to reconnect");
            break;

        case MQTT_EVENT_DATA:
            if (s_message_handler != NULL) {
                topic_len = event->topic_len;
                data_len = event->data_len;

                if (event->topic == NULL || event->data == NULL ||
                    topic_len <= 0 || topic_len >= BUFFER_SIZE ||
                    data_len <= 0 || data_len >= BUFFER_SIZE) {
                    ESP_LOGW(TAG, "Ignoring MQTT message with invalid or oversized lengths");
                    break;
                }

                if (event->current_data_offset != 0 ||
                    event->total_data_len != data_len) {
                    ESP_LOGW(TAG, "Ignoring fragmented MQTT message (offset=%d, part=%d, total=%d)",
                             event->current_data_offset, data_len, event->total_data_len);
                    break;
                }

                memcpy(topic_buf, event->topic, topic_len);
                topic_buf[topic_len] = '\0';
                memcpy(data_buf, event->data, data_len);
                data_buf[data_len] = '\0';

                ESP_LOGI(TAG, "Topic: %s, Data: %s", topic_buf, data_buf);
                s_message_handler(topic_buf, data_buf);
            }
            break;

        case MQTT_EVENT_ERROR:
            log_mqtt_error(event->error_handle);
            break;

        default:
            break;
    }
}

esp_err_t mqtt_app_start(void)
{
    // Не допускаємо повторної ініціалізації вже запущеного клієнта.
    if (s_client != NULL) {
        ESP_LOGE(TAG, "MQTT client has already been initialized");
        return ESP_ERR_INVALID_STATE;
    }

    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = MQTT_BROKER_URI;

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }

    esp_err_t err = esp_mqtt_client_register_event(s_client, MQTT_EVENT_ANY,
                                                    mqtt_event_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register MQTT event handler: %s (0x%x)",
                 esp_err_to_name(err), (unsigned int)err);
        // Звільняємо клієнт, якщо будь-який крок запуску завершився помилкою.
        destroy_mqtt_client();
        return err;
    }

    err = esp_mqtt_client_start(s_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client: %s (0x%x)",
                 esp_err_to_name(err), (unsigned int)err);
        destroy_mqtt_client();
        return err;
    }

    return ESP_OK;
}

esp_mqtt_client_handle_t mqtt_get_client(void)
{
    return s_client;
}

bool mqtt_is_connected(void)
{
    return s_connected;
}

void mqtt_set_message_handler(mqtt_message_handler_t handler)
{
    s_message_handler = handler;
}
