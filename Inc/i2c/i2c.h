#ifndef I2C_H
#define I2C_H

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

HAL_StatusTypeDef I2C_Init(I2C_HandleTypeDef *hi2c,
                           I2C_TypeDef *instance,
                           uint32_t clock_speed);
/* device_address is a 7-bit address; the module applies HAL's required shift. */
HAL_StatusTypeDef I2C_Transmit(I2C_HandleTypeDef *hi2c,
                               uint16_t device_address,
                               uint8_t *data,
                               uint16_t size,
                               uint32_t timeout);
HAL_StatusTypeDef I2C_Probe(I2C_HandleTypeDef *hi2c,
                            uint16_t device_address,
                            uint32_t timeout);
/* Scans 7-bit addresses 0x08..0x77 and stores responding ones in found[].
   *count receives the number of responding devices, even if it exceeds max_found. */
HAL_StatusTypeDef I2C_Scan(I2C_HandleTypeDef *hi2c,
                           uint8_t *found,
                           uint8_t max_found,
                           uint8_t *count,
                           uint32_t timeout);
HAL_StatusTypeDef I2C_ReadRegister(I2C_HandleTypeDef *hi2c,
                                   uint16_t device_address,
                                   uint8_t reg,
                                   uint8_t *data,
                                   uint16_t size,
                                   uint32_t timeout);
HAL_StatusTypeDef I2C_Receive(I2C_HandleTypeDef *hi2c,
                              uint16_t device_address,
                              uint8_t *data,
                              uint16_t size,
                              uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* I2C_H */
