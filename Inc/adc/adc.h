#ifndef ADC_H
#define ADC_H

#include "stm32f4xx_hal.h"
// Ініціалізація каналів АЦП
HAL_StatusTypeDef ADC_Init(const uint32_t *channels, uint32_t count);

// Зчитування послідовності значень з АЦП
HAL_StatusTypeDef ADC_ReadSequence(uint32_t *values, uint32_t count);

#endif
