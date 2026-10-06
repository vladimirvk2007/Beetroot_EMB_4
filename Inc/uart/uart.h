#ifndef UART_H
#define UART_H

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

HAL_StatusTypeDef UART_Init(UART_HandleTypeDef *huart,
                            USART_TypeDef *instance,
                            uint32_t baud_rate);
HAL_StatusTypeDef UART_Transmit(UART_HandleTypeDef *huart,
                                uint8_t *data,
                                uint16_t size,
                                uint32_t timeout);
HAL_StatusTypeDef UART_Receive(UART_HandleTypeDef *huart,
                               uint8_t *data,
                               uint16_t size,
                               uint32_t timeout);
void UART_IRQHandler(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif /* UART_H */
