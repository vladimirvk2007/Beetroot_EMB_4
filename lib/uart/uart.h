#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t uart_init(void);
esp_err_t uart_receive(uint8_t *data, size_t size, uint32_t timeout_ms);
esp_err_t uart_transmit(const uint8_t *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif
