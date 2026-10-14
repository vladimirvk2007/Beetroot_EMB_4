#include "spi_master/spi_master.h"

static uint8_t SPI_Master_IsValidPrescaler(uint32_t baud_prescaler)
{
  return (baud_prescaler == SPI_BAUDRATEPRESCALER_2) ||
         (baud_prescaler == SPI_BAUDRATEPRESCALER_4) ||
         (baud_prescaler == SPI_BAUDRATEPRESCALER_8) ||
         (baud_prescaler == SPI_BAUDRATEPRESCALER_16) ||
         (baud_prescaler == SPI_BAUDRATEPRESCALER_32) ||
         (baud_prescaler == SPI_BAUDRATEPRESCALER_64) ||
         (baud_prescaler == SPI_BAUDRATEPRESCALER_128) ||
         (baud_prescaler == SPI_BAUDRATEPRESCALER_256);
}

HAL_StatusTypeDef SPI_Master_Init(SPI_HandleTypeDef *hspi,
                                  SPI_TypeDef *instance,
                                  uint32_t baud_prescaler)
{
  if ((hspi == NULL) || (instance != SPI1) ||
      !SPI_Master_IsValidPrescaler(baud_prescaler))
  {
    return HAL_ERROR;
  }

  hspi->Instance = instance;
  hspi->Init.Mode = SPI_MODE_MASTER;
  hspi->Init.Direction = SPI_DIRECTION_2LINES;
  hspi->Init.DataSize = SPI_DATASIZE_8BIT;
  hspi->Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi->Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi->Init.NSS = SPI_NSS_SOFT;
  hspi->Init.BaudRatePrescaler = baud_prescaler;
  hspi->Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi->Init.TIMode = SPI_TIMODE_DISABLE;
  hspi->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi->Init.CRCPolynomial = 10;

  return HAL_SPI_Init(hspi);
}

HAL_StatusTypeDef SPI_Master_DeInit(SPI_HandleTypeDef *hspi)
{
  if ((hspi == NULL) || (hspi->Instance != SPI1))
  {
    return HAL_ERROR;
  }

  return HAL_SPI_DeInit(hspi);
}

HAL_StatusTypeDef SPI_Master_Transmit(SPI_HandleTypeDef *hspi,
                                       uint8_t *data,
                                       uint16_t size,
                                       uint32_t timeout)
{
  if ((hspi == NULL) || (data == NULL) || (size == 0))
  {
    return HAL_ERROR;
  }

  return HAL_SPI_Transmit(hspi, data, size, timeout);
}

HAL_StatusTypeDef SPI_Master_Receive(SPI_HandleTypeDef *hspi,
                                      uint8_t *data,
                                      uint16_t size,
                                      uint32_t timeout)
{
  if ((hspi == NULL) || (data == NULL) || (size == 0))
  {
    return HAL_ERROR;
  }

  return HAL_SPI_Receive(hspi, data, size, timeout);
}

HAL_StatusTypeDef SPI_Master_TransmitReceive(SPI_HandleTypeDef *hspi,
                                              uint8_t *tx_data,
                                              uint8_t *rx_data,
                                              uint16_t size,
                                              uint32_t timeout)
{
  if ((hspi == NULL) || (tx_data == NULL) || (rx_data == NULL) ||
      (size == 0))
  {
    return HAL_ERROR;
  }

  return HAL_SPI_TransmitReceive(hspi, tx_data, rx_data, size, timeout);
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
  GPIO_InitTypeDef gpio_init = {0};

  if (hspi == NULL)
  {
    return;
  }

  gpio_init.Mode = GPIO_MODE_AF_PP;
  gpio_init.Pull = GPIO_NOPULL;
  gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

  if (hspi->Instance == SPI1)
  {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();

    gpio_init.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    gpio_init.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &gpio_init);
  }
  else if (hspi->Instance == SPI2)
  {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_SPI2_CLK_ENABLE();

    gpio_init.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    gpio_init.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &gpio_init);
  }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi)
{
  if (hspi == NULL)
  {
    return;
  }

  if (hspi->Instance == SPI1)
  {
    __HAL_RCC_SPI1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
  }
  else if (hspi->Instance == SPI2)
  {
    __HAL_RCC_SPI2_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_12 | GPIO_PIN_13 |
                           GPIO_PIN_14 | GPIO_PIN_15);
  }
}
