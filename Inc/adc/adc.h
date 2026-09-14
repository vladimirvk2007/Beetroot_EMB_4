#ifndef ADC_H
#define ADC_H

#include "stm32f4xx_hal.h"

HAL_StatusTypeDef ADC_Read(uint32_t *value, uint32_t timeout);
HAL_StatusTypeDef ADC_ReadChannel(uint32_t channel, uint32_t *value, uint32_t timeout);
HAL_StatusTypeDef ADC_ReadSequence(uint32_t *values, uint32_t count, uint32_t timeout);

#endif
