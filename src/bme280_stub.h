#ifndef BME280_STUB_H
#define BME280_STUB_H

#include <stdint.h>

#include "esp_err.h"

esp_err_t bme280_read_stub(int16_t* temp_x100, uint16_t* hum_x100, uint16_t* pressure_mmhg);

#endif // BME280_STUB_H
