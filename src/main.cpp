#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "credentials.h"
#include "wifi_setup.h"
#include "mqtt.h"

static const char *TAG = "main";

#define LED_GPIO             GPIO_NUM_16
#define PUBLISH_INTERVAL_MS  10000

extern "C" void app_main(void)
{
    // NVS required by Wi-Fi driver
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Configure LED GPIO
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask  = (1ULL << LED_GPIO);
    io_conf.mode          = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en    = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en  = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type     = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
    gpio_set_level(LED_GPIO, 0);

    wifi_init_sta();
    mqtt_app_start();

    uint32_t count = 0;
    char msg[64];

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(PUBLISH_INTERVAL_MS));
        count++;
        snprintf(msg, sizeof(msg), "Hello from ESP32-S3! #%" PRIu32, count);

        esp_mqtt_client_handle_t client = mqtt_get_client();
        if (client) {
            esp_mqtt_client_publish(client, MQTT_TOPIC, msg, 0, 0, 0);
            ESP_LOGI(TAG, "Published: %s", msg);
        }
    }
}

