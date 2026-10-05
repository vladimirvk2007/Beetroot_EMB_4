#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "uart.h"

static const char *TAG = "app";

extern "C" void app_main()
{
    const esp_err_t init_err = uart_init();
    if (init_err != ESP_OK) {
        ESP_LOGE(TAG, "UART initialization failed: %s", esp_err_to_name(init_err));
        return;
    }

    ESP_LOGI(TAG, "UART echo application started");

    uint8_t received_byte;
    while (1) {
        const esp_err_t receive_err = uart_receive(&received_byte, 1, 0);
        if (receive_err == ESP_OK) {
            ESP_LOGI(TAG, "UART RX: 0x%02X \"%c\"",
                     (unsigned int)received_byte, (int)received_byte);

            const esp_err_t transmit_err = uart_transmit(&received_byte, 1);
            if (transmit_err != ESP_OK) {
                ESP_LOGE(TAG, "UART transmit error: %s", esp_err_to_name(transmit_err));
            }
        } else if (receive_err != ESP_ERR_TIMEOUT) {
            ESP_LOGE(TAG, "UART receive error: %s", esp_err_to_name(receive_err));
        }

        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}
