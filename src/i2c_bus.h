#ifndef I2C_BUS_H
#define I2C_BUS_H

#include <stdint.h>

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

esp_err_t i2c_bus_init(i2c_port_t port, gpio_num_t sda_pin, gpio_num_t scl_pin, uint32_t freq_hz);
esp_err_t i2c_bus_ping_address(i2c_port_t port, uint8_t addr, TickType_t timeout);
void i2c_bus_scan(i2c_port_t port, TickType_t timeout, const char* log_tag);

#endif // I2C_BUS_H
