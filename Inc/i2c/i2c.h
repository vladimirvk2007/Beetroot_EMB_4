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
