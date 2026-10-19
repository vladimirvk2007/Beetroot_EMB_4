#ifndef ADC_H
#define ADC_H

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

// Ініціалізація каналів АЦП
HAL_StatusTypeDef ADC_Init(const uint32_t *channels, uint32_t count);

// Асинхронний запуск послідовності перетворень через DMA
HAL_StatusTypeDef ADC_StartSequenceDMA(uint32_t *values, uint32_t count);

// Зчитування послідовності значень з АЦП через DMA
HAL_StatusTypeDef ADC_ReadSequence(uint32_t *values, uint32_t count);

// Обробник переривання DMA2 Stream 0
void ADC_DMA_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif
