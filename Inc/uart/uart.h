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

/* DMA-обмін (USART1: TX - DMA2 Stream7, RX - DMA2 Stream2). Без блокування:
 * завершення повідомляється через колбеки (викликаються з IRQ).
 * Буфер має бути валідним до завершення передачі. */
typedef void (*UART_DMA_Callback)(UART_HandleTypeDef *huart);

/* Реєструє колбеки завершення передачі (TX) та прийому (RX) */
void UART_RegisterDMACallbacks(UART_DMA_Callback tx_complete,
                               UART_DMA_Callback rx_complete);
/* Запускає передачу size байт через DMA */
HAL_StatusTypeDef UART_Transmit_DMA(UART_HandleTypeDef *huart,
                                    uint8_t *data,
                                    uint16_t size);
/* Запускає прийом size байт через DMA */
HAL_StatusTypeDef UART_Receive_DMA(UART_HandleTypeDef *huart,
                                   uint8_t *data,
                                   uint16_t size);
/* Перериває поточні DMA-передачі (TX і RX) */
HAL_StatusTypeDef UART_DMA_Abort(UART_HandleTypeDef *huart);
/* Обробник переривання UART (викликає HAL) */
void UART_IRQHandler(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif /* UART_H */
