#include "uart/uart.h"
#include "main.h"

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

static DMA_HandleTypeDef hdma_usart1_tx;
static DMA_HandleTypeDef hdma_usart1_rx;
static UART_HandleTypeDef *uart1_handle = NULL;
static UART_DMA_Callback tx_complete_cb = NULL;
static UART_DMA_Callback rx_complete_cb = NULL;

/* Налаштовує DMA-потоки TX/RX для USART1 і вмикає їхні переривання */
static HAL_StatusTypeDef UART_DMA_Setup(UART_HandleTypeDef *huart)
{
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* USART1_TX: DMA2 Stream7 Channel4 */
  hdma_usart1_tx.Instance = DMA2_Stream7;
  hdma_usart1_tx.Init.Channel = DMA_CHANNEL_4;
  hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_usart1_tx.Init.Mode = DMA_NORMAL;
  hdma_usart1_tx.Init.Priority = DMA_PRIORITY_LOW;
  hdma_usart1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK)
  {
    return HAL_ERROR;
  }
  __HAL_LINKDMA(huart, hdmatx, hdma_usart1_tx);

  /* USART1_RX: DMA2 Stream2 Channel4 */
  hdma_usart1_rx.Instance = DMA2_Stream2;
  hdma_usart1_rx.Init.Channel = DMA_CHANNEL_4;
  hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_usart1_rx.Init.Mode = DMA_NORMAL;
  hdma_usart1_rx.Init.Priority = DMA_PRIORITY_LOW;
  hdma_usart1_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if (HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK)
  {
    return HAL_ERROR;
  }
  __HAL_LINKDMA(huart, hdmarx, hdma_usart1_rx);

  HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
  HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);

  return HAL_OK;
}

/* Зберігає користувацькі колбеки завершення TX/RX */
void UART_RegisterDMACallbacks(UART_DMA_Callback tx_complete,
                               UART_DMA_Callback rx_complete)
{
  tx_complete_cb = tx_complete;
  rx_complete_cb = rx_complete;
}

/* Неблокуюча передача через DMA з перевіркою аргументів */
HAL_StatusTypeDef UART_Transmit_DMA(UART_HandleTypeDef *huart,
                                    uint8_t *data,
                                    uint16_t size)
{
  if ((huart == NULL) || (data == NULL) || (size == 0))
  {
    return HAL_ERROR;
  }

  return HAL_UART_Transmit_DMA(huart, data, size);
}

/* Неблокуючий прийом через DMA з перевіркою аргументів */
HAL_StatusTypeDef UART_Receive_DMA(UART_HandleTypeDef *huart,
                                   uint8_t *data,
                                   uint16_t size)
{
  if ((huart == NULL) || (data == NULL) || (size == 0))
  {
    return HAL_ERROR;
  }

  return HAL_UART_Receive_DMA(huart, data, size);
}

/* Перериває активні передачі UART/DMA */
HAL_StatusTypeDef UART_DMA_Abort(UART_HandleTypeDef *huart)
{
  if (huart == NULL)
  {
    return HAL_ERROR;
  }

  return HAL_UART_Abort(huart);
}

/* HAL-колбек завершення TX: передає подію користувацькому колбеку */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if ((huart->Instance == USART1) && (tx_complete_cb != NULL))
  {
    tx_complete_cb(huart);
  }
}

/* HAL-колбек завершення RX: передає подію користувацькому колбеку */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if ((huart->Instance == USART1) && (rx_complete_cb != NULL))
  {
    rx_complete_cb(huart);
  }
}

/* Переривання DMA TX (USART1) */
void DMA2_Stream7_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_usart1_tx);
}

/* Переривання DMA RX (USART1) */
void DMA2_Stream2_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_usart1_rx);
}

/* Переривання USART1 (потрібне для завершення TX через DMA) */
void USART1_IRQHandler(void)
{
  UART_IRQHandler(uart1_handle);
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
  GPIO_InitTypeDef gpio_init = {0};

  if (huart->Instance != USART1)
  {
    return;
  }

  uart1_handle = huart;

  __HAL_RCC_USART1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  gpio_init.Pin = GPIO_PIN_9 | GPIO_PIN_10;
  gpio_init.Mode = GPIO_MODE_AF_PP;
  gpio_init.Pull = GPIO_NOPULL;
  gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_init.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOA, &gpio_init);

  if (UART_DMA_Setup(huart) != HAL_OK)
  {
    Error_Handler();
  }

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
  HAL_DMA_DeInit(huart->hdmatx);
  HAL_DMA_DeInit(huart->hdmarx);
  HAL_NVIC_DisableIRQ(DMA2_Stream7_IRQn);
  HAL_NVIC_DisableIRQ(DMA2_Stream2_IRQn);
  HAL_NVIC_DisableIRQ(USART1_IRQn);
  uart1_handle = NULL;
}

void UART_IRQHandler(UART_HandleTypeDef *huart)
{
  if (huart != NULL)
  {
    HAL_UART_IRQHandler(huart);
  }
}
