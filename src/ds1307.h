#ifndef DS1307_H
#define DS1307_H

#include <stdint.h>

#include "driver/i2c.h"
#include "esp_err.h"
#include "freertos/semphr.h"

#include "app_types.h"

esp_err_t ds1307_read_time(i2c_port_t port, uint8_t address, SemaphoreHandle_t i2c_mutex, struct RtcDateTime* out);

#endif // DS1307_H
