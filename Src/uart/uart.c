#include "uart/uart.h"

HAL_StatusTypeDef UART_Init(UART_HandleTypeDef *huart,
                            USART_TypeDef *instance,
                            uint32_t baud_rate)
{
  if ((huart == NULL) || (instance != USART1) || (baud_rate == 0))
  {
    return HAL_ERROR;
  }

  huart->Instance = instance;
  huart->Init.BaudRate = baud_rate;
  huart->Init.WordLength = UART_WORDLENGTH_8B;
  huart->Init.StopBits = UART_STOPBITS_1;
  huart->Init.Parity = UART_PARITY_NONE;
  huart->Init.Mode = UART_MODE_TX_RX;
  huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart->Init.OverSampling = UART_OVERSAMPLING_16;

  return HAL_UART_Init(huart);
}

HAL_StatusTypeDef UART_Transmit(UART_HandleTypeDef *huart,
                                uint8_t *data,
                                uint16_t size,
                                uint32_t timeout)
{
  if ((huart == NULL) || (data == NULL) || (size == 0))
  {
    return HAL_ERROR;
  }

  return HAL_UART_Transmit(huart, data, size, timeout);
}

HAL_StatusTypeDef UART_Receive(UART_HandleTypeDef *huart,
                               uint8_t *data,
                               uint16_t size,
                               uint32_t timeout)
{
  if ((huart == NULL) || (data == NULL) || (size == 0))
  {
    return HAL_ERROR;
  }

  return HAL_UART_Receive(huart, data, size, timeout);
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
  GPIO_InitTypeDef gpio_init = {0};

  if (huart->Instance != USART1)
  {
    return;
  }

  __HAL_RCC_USART1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  gpio_init.Pin = GPIO_PIN_9 | GPIO_PIN_10;
  gpio_init.Mode = GPIO_MODE_AF_PP;
  gpio_init.Pull = GPIO_NOPULL;
  gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_init.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOA, &gpio_init);

  HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
  if (huart->Instance != USART1)
  {
    return;
  }

  __HAL_RCC_USART1_CLK_DISABLE();
  HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);
  HAL_NVIC_DisableIRQ(USART1_IRQn);
}

void UART_IRQHandler(UART_HandleTypeDef *huart)
{
  if (huart != NULL)
  {
    HAL_UART_IRQHandler(huart);
  }
}
