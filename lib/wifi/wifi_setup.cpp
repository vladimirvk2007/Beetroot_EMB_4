#include "wifi_setup.h"
#include "credentials.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include <string.h>

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1
#define WIFI_MAX_RETRY      10

static const char *TAG = "wifi";

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static bool s_connected_once = false;

// Перевіряє код ініціалізації та повертає його виклику після запису в лог.
#define RETURN_ON_ERROR(operation) do { \
    esp_err_t err = (operation); \
    if (err != ESP_OK) { \
        ESP_LOGE(TAG, "%s failed: %s (0x%x)", #operation, esp_err_to_name(err), \
                 (unsigned int)err); \
        return err; \
    } \
} while (0)

static void event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // Починаємо підключення після запуску Wi-Fi драйвера.
        esp_err_t err = esp_wifi_connect();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Initial WiFi connection request failed: %s (0x%x)",
                     esp_err_to_name(err), (unsigned int)err);
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *event =
            (wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGW(TAG, "Disconnected from AP (reason=%d)", event->reason);

        // До першого підключення спроби обмежені; після нього перепідключаємось постійно.
        if (s_connected_once || s_retry_num < WIFI_MAX_RETRY) {
            esp_err_t err = esp_wifi_connect();
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "WiFi reconnect request failed: %s (0x%x)",
                         esp_err_to_name(err), (unsigned int)err);
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            }
            if (s_connected_once) {
                ESP_LOGI(TAG, "Reconnecting to WiFi");
            } else {
                s_retry_num++;
                ESP_LOGI(TAG, "Retry WiFi connection (%d/%d)",
                         s_retry_num, WIFI_MAX_RETRY);
            }
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "WiFi connection failed after %d retries", WIFI_MAX_RETRY);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        s_connected_once = true;
        // Розбуджуємо wifi_init_sta після успішного отримання адреси.
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_init_sta(void)
{
    wifi_config_t wifi_config = {};
    // Перевіряємо довжину до копіювання в обмежені буфери ESP-IDF.
    if (strlen(WIFI_SSID) > sizeof(wifi_config.sta.ssid) ||
        strlen(WIFI_PASSWORD) >= sizeof(wifi_config.sta.password)) {
        ESP_LOGE(TAG, "WiFi SSID or password exceeds the supported length");
        return ESP_ERR_INVALID_SIZE;
    }

    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create WiFi event group");
        return ESP_ERR_NO_MEM;
    }

    RETURN_ON_ERROR(esp_netif_init());
    RETURN_ON_ERROR(esp_event_loop_create_default());
    if (esp_netif_create_default_wifi_sta() == NULL) {
        ESP_LOGE(TAG, "Failed to create default WiFi STA netif");
        return ESP_FAIL;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    RETURN_ON_ERROR(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    RETURN_ON_ERROR(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        &event_handler, NULL, &instance_any_id));
    RETURN_ON_ERROR(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                        &event_handler, NULL, &instance_got_ip));

    memcpy(wifi_config.sta.ssid,     WIFI_SSID,     strlen(WIFI_SSID));
    memcpy(wifi_config.sta.password, WIFI_PASSWORD, strlen(WIFI_PASSWORD));

    RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA));
    RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    RETURN_ON_ERROR(esp_wifi_start());
    // Вимикаємо енергозбереження Wi-Fi для стабільнішого зв'язку.
    RETURN_ON_ERROR(esp_wifi_set_ps(WIFI_PS_NONE));

    // Очікуємо результат асинхронного підключення з обробника подій.
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                            pdFALSE, pdFALSE, portMAX_DELAY);
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP: %s", WIFI_SSID);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Connection to AP failed: %s", WIFI_SSID);
        return ESP_FAIL;
    }
}
