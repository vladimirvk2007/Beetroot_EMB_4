#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#include "driver/i2c.h"
#include "esp_err.h"

#include "app_types.h"

esp_err_t display_init(i2c_port_t port, uint8_t address);
esp_err_t display_render_sample(i2c_port_t port, uint8_t address, const SensorData* sample);

#endif // DISPLAY_H
