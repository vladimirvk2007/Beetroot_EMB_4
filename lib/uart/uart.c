#include "uart.h"

#include <limits.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#define UART_PORT UART_NUM_1
#define UART_BAUD_RATE 115200
#define UART_TX_PIN GPIO_NUM_17
#define UART_RX_PIN GPIO_NUM_18
#define UART_RX_BUFFER_SIZE 256
#define UART_TX_BUFFER_SIZE 0

static const char *TAG = "uart";

esp_err_t uart_init(void)
{
    const uart_config_t config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err = uart_driver_install(
        UART_PORT,
        UART_RX_BUFFER_SIZE,
        UART_TX_BUFFER_SIZE,
        0,
        NULL,
        0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART driver install failed: %s", esp_err_to_name(err));
        return err;
    }

    err = uart_param_config(UART_PORT, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART configuration failed: %s", esp_err_to_name(err));
        uart_driver_delete(UART_PORT);
        return err;
    }

    err = uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART pin configuration failed: %s", esp_err_to_name(err));
        uart_driver_delete(UART_PORT);
        return err;
    }

    ESP_LOGI(TAG, "UART1 ready: %d baud, 8N1, TX GPIO%d, RX GPIO%d",
             UART_BAUD_RATE, UART_TX_PIN, UART_RX_PIN);
    return ESP_OK;
}

esp_err_t uart_receive(uint8_t *data, size_t size, uint32_t timeout_ms)
{
    if (data == NULL || size == 0U || size > INT_MAX) {
        ESP_LOGE(TAG, "Invalid UART receive arguments");
        return ESP_ERR_INVALID_ARG;
    }

    const int bytes_read = uart_read_bytes(
        UART_PORT,
        data,
        size,
        pdMS_TO_TICKS(timeout_ms));
    if (bytes_read < 0) {
        ESP_LOGE(TAG, "UART receive failed");
        return ESP_FAIL;
    }

    return (bytes_read == 0) ? ESP_ERR_TIMEOUT : ESP_OK;
}

esp_err_t uart_transmit(const uint8_t *data, size_t size)
{
    if (data == NULL || size == 0U || size > INT_MAX) {
        ESP_LOGE(TAG, "Invalid UART transmit arguments");
        return ESP_ERR_INVALID_ARG;
    }

    const int bytes_written = uart_write_bytes(UART_PORT, data, size);
    if (bytes_written < 0) {
        ESP_LOGE(TAG, "UART transmit failed");
        return ESP_FAIL;
    }

    return ((size_t)bytes_written == size) ? ESP_OK : ESP_FAIL;
}
