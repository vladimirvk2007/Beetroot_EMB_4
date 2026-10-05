#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "uart/uart.h"



extern "C" void app_main() {
    if (uart_init() != ESP_OK) {
        return;
    }

    uint8_t received_byte;
    while (1) {
        const int bytes_read = uart_receive(&received_byte, 1, 0);
        if (bytes_read < 0) {
            printf("UART receive error: %d\n", bytes_read);
        } else if (bytes_read > 0) {
            const int bytes_written = uart_transmit(&received_byte, 1);
            if (bytes_written != 1) {
                printf("UART transmit error: %d\n", bytes_written);
            }
        }

        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}
