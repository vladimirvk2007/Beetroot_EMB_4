#ifndef AT24C32_H
#define AT24C32_H

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* AT24C32: 32 Kbit (4096 B), сторінка 32 B, 2-байтна адреса, адреса I2C 0x50 + A2..A0 */
#define AT24C32_BASE_ADDRESS        0x50U
#define AT24C32_SIZE_BYTES          4096UL
#define AT24C32_PAGE_SIZE_BYTES     32U
#define AT24C32_DEFAULT_TIMEOUT_MS  100U
#define AT24C32_WRITE_CYCLE_MS      20U  /* tWR max 10 мс + запас */

/* Поля можна змінити після AT24C32_Init(), щоб працювати з іншими EEPROM
   серії AT24Cxx з 16-бітною адресою (наприклад AT24C64: size 8192, page 32). */
typedef struct
{
  I2C_HandleTypeDef *hi2c;
  uint8_t device_address;      /* 7-бітна адреса I2C */
  uint32_t size;               /* ємність, байт */
  uint16_t page_size;          /* розмір сторінки запису, байт */
  uint32_t timeout;            /* таймаут передачі I2C, мс */
  uint32_t write_cycle_timeout;/* максимальне очікування завершення запису, мс */
} AT24C32_HandleTypeDef;

/* a_pins: стан виводів A2..A0 (0..7). */
HAL_StatusTypeDef AT24C32_Init(AT24C32_HandleTypeDef *dev,
                               I2C_HandleTypeDef *hi2c,
                               uint8_t a_pins);

/* Перевіряє, що мікросхема відповідає на шині. */
HAL_StatusTypeDef AT24C32_IsReady(AT24C32_HandleTypeDef *dev);

HAL_StatusTypeDef AT24C32_Read(AT24C32_HandleTypeDef *dev,
                               uint32_t address,
                               uint8_t *data,
                               uint32_t size);

/* Запис довільної довжини; автоматично розбивається по межах сторінок. */
HAL_StatusTypeDef AT24C32_Write(AT24C32_HandleTypeDef *dev,
                                uint32_t address,
                                const uint8_t *data,
                                uint32_t size);

HAL_StatusTypeDef AT24C32_ReadByte(AT24C32_HandleTypeDef *dev,
                                   uint32_t address,
                                   uint8_t *value);

HAL_StatusTypeDef AT24C32_WriteByte(AT24C32_HandleTypeDef *dev,
                                    uint32_t address,
                                    uint8_t value);

/* Заповнює діапазон значенням (0xFF - стан після очищення). */
HAL_StatusTypeDef AT24C32_Fill(AT24C32_HandleTypeDef *dev,
                               uint32_t address,
                               uint32_t size,
                               uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* AT24C32_H */
