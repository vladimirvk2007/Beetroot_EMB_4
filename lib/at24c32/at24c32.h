#ifndef AT24C32_H
#define AT24C32_H

#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AT24C32_BASE_ADDRESS       0x50U
#define AT24C32_SIZE_BYTES         4096U
#define AT24C32_PAGE_SIZE_BYTES    32U
#define AT24C32_I2C_SPEED_HZ       100000U
#define AT24C32_I2C_TIMEOUT_MS     1000U
#define AT24C32_WRITE_CYCLE_MS     20U

typedef struct at24c32_s *at24c32_handle_t;

/*
 * Додає AT24C32 до ініціалізованої шини I2C.
 * a_pins задає рівні виводів A2..A0 (0..7).
 */
esp_err_t at24c32_init(i2c_master_bus_handle_t bus,
                       uint8_t a_pins,
                       at24c32_handle_t *memory);

/* Видаляє пристрій; шина I2C залишається ініціалізованою. */
esp_err_t at24c32_deinit(at24c32_handle_t memory);

/* Перевіряє, чи відповідає EEPROM на свою адресу I2C. */
esp_err_t at24c32_is_ready(at24c32_handle_t memory);

/* Читає або записує діапазон байтів у межах адресного простору 4096 байтів. */
esp_err_t at24c32_read(at24c32_handle_t memory,
                       uint32_t address,
                       uint8_t *data,
                       uint32_t size);
esp_err_t at24c32_write(at24c32_handle_t memory,
                        uint32_t address,
                        const uint8_t *data,
                        uint32_t size);

/* Читає або записує повну сторінку за адресою, вирівняною на 32 байти. */
esp_err_t at24c32_read_page(at24c32_handle_t memory,
                            uint32_t page_address,
                            uint8_t data[AT24C32_PAGE_SIZE_BYTES]);
esp_err_t at24c32_write_page(at24c32_handle_t memory,
                             uint32_t page_address,
                             const uint8_t data[AT24C32_PAGE_SIZE_BYTES]);

esp_err_t at24c32_read_byte(at24c32_handle_t memory,
                            uint32_t address,
                            uint8_t *value);
esp_err_t at24c32_write_byte(at24c32_handle_t memory,
                             uint32_t address,
                             uint8_t value);

/* Заповнює діапазон значенням; 0xFF відповідає стертій пам'яті. */
esp_err_t at24c32_fill(at24c32_handle_t memory,
                       uint32_t address,
                       uint32_t size,
                       uint8_t value);

#ifdef __cplusplus
}
#endif

#endif
