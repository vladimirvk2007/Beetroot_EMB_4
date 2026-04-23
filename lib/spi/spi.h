#ifndef SPI_H
#define SPI_H

#include "driver/spi_master.h"
#include "esp_err.h"

esp_err_t spi_initialize(spi_device_handle_t *spi);
esp_err_t spi_read_registers(spi_device_handle_t spi, uint8_t reg,
                              uint8_t *rx_buf, size_t num_bytes);
esp_err_t spi_write_register(spi_device_handle_t spi, uint8_t reg, uint8_t value);

#endif // SPI_H

