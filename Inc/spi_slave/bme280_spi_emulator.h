#ifndef BME280_SPI_EMULATOR_H
#define BME280_SPI_EMULATOR_H

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Карта адрес BME280 зберігається у 7-бітному форматі. */
#define BME280_SPI_EMULATOR_REGISTER_COUNT 128U
#define BME280_SPI_EMULATOR_CHIP_ID 0x60U
#define BME280_SPI_EMULATOR_CHIP_ID_REGISTER 0xD0U

/* Стан емулятора та буфери SPI-обміну. */
typedef struct
{
  SPI_HandleTypeDef *hspi;
  volatile uint8_t registers[BME280_SPI_EMULATOR_REGISTER_COUNT];
  volatile uint8_t address;
  volatile uint8_t is_read;
  volatile uint8_t command_received;
  volatile uint8_t tx_byte;
  volatile uint8_t rx_byte;
  volatile HAL_StatusTypeDef last_status;
} BME280_SPI_Emulator_HandleTypeDef;

/* Ініціалізація, запуск і зупинка обробки SPI2. */
HAL_StatusTypeDef BME280_SPI_Emulator_Init(
    BME280_SPI_Emulator_HandleTypeDef *emulator,
    SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef BME280_SPI_Emulator_Start(
    BME280_SPI_Emulator_HandleTypeDef *emulator);
HAL_StatusTypeDef BME280_SPI_Emulator_Stop(
    BME280_SPI_Emulator_HandleTypeDef *emulator);

/* Читання та зміна значень регістрів емулятора. */
HAL_StatusTypeDef BME280_SPI_Emulator_GetRegister(
    BME280_SPI_Emulator_HandleTypeDef *emulator,
    uint8_t address,
    uint8_t *value);
HAL_StatusTypeDef BME280_SPI_Emulator_SetRegister(
    BME280_SPI_Emulator_HandleTypeDef *emulator,
    uint8_t address,
    uint8_t value);
HAL_StatusTypeDef BME280_SPI_Emulator_GetStatus(
    BME280_SPI_Emulator_HandleTypeDef *emulator);

/* Виклики з обробників SPI2 та NSS (PB12). */
void BME280_SPI_Emulator_IRQHandler(void);
void BME280_SPI_Emulator_NssChanged(void);

#ifdef __cplusplus
}
#endif

#endif /* BME280_SPI_EMULATOR_H */
