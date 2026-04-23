#ifndef BME280_SPI_H
#define BME280_SPI_H

#include "esp_err.h"
#include "driver/spi_master.h"

struct Bme280Calib {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;

    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;

    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;
};

struct Bme280Compensated {
    float temperature_c;
    float pressure_hpa;
    float humidity_rh;
};

esp_err_t bme280_read_calibration(spi_device_handle_t spi, Bme280Calib *cal);
Bme280Compensated bme280_compensate(int32_t raw_temp,
                                           int32_t raw_press,
                                           int32_t raw_hum,
                                           const Bme280Calib *cal);

#endif // BME280_SPI_H

