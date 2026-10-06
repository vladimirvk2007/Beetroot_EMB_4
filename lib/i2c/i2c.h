#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    i2c_port_num_t port;
    gpio_num_t sda_pin;
    gpio_num_t scl_pin;
    bool enable_internal_pullup;
} i2c_bus_settings_t;


typedef struct {
    uint16_t address;
    uint32_t scl_speed_hz; /* 0 = 100 kHz */
} i2c_device_settings_t;

esp_err_t i2c_bus_init(const i2c_bus_settings_t *settings, i2c_master_bus_handle_t *bus);
esp_err_t i2c_bus_deinit(i2c_master_bus_handle_t bus);
esp_err_t i2c_bus_probe(i2c_master_bus_handle_t bus, uint16_t address, uint32_t timeout_ms);

esp_err_t i2c_device_add(i2c_master_bus_handle_t bus,
                         const i2c_device_settings_t *settings,
                         i2c_master_dev_handle_t *device);
esp_err_t i2c_device_remove(i2c_master_dev_handle_t device);

esp_err_t i2c_write(i2c_master_dev_handle_t device,
                    const uint8_t *data,
                    size_t size,
                    uint32_t timeout_ms);
esp_err_t i2c_read(i2c_master_dev_handle_t device,
                   uint8_t *data,
                   size_t size,
                   uint32_t timeout_ms);
esp_err_t i2c_write_read(i2c_master_dev_handle_t device,
                         const uint8_t *write_data,
                         size_t write_size,
                         uint8_t *read_data,
                         size_t read_size,
                         uint32_t timeout_ms);

esp_err_t i2c_write_register(i2c_master_dev_handle_t device,
                             uint8_t reg,
                             const uint8_t *data,
                             size_t size,
                             uint32_t timeout_ms);
esp_err_t i2c_read_register(i2c_master_dev_handle_t device,
                            uint8_t reg,
                            uint8_t *data,
                            size_t size,
                            uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
