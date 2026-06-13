#ifndef UART_CONSOLE_H
#define UART_CONSOLE_H

#include <stdbool.h>
#include <stdint.h>

#include "driver/i2c.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "app_types.h"

struct UartConsoleContext {
    i2c_port_t i2c_port;
    uint8_t ds1307_addr;
    SemaphoreHandle_t i2c_mutex;

    SensorData* last_sample;
    bool* has_sample;
    bool* display_ready;
    bool* bme_ready;
    esp_err_t* last_rtc_err;
    SemaphoreHandle_t state_mutex;
};

void uart_console_task(void* pvParameters);

#endif // UART_CONSOLE_H
