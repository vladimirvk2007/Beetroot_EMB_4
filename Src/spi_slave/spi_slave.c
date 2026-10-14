#include "spi_slave/spi_slave.h"

HAL_StatusTypeDef SPI_Slave_Init(SPI_HandleTypeDef *hspi,
                                 SPI_TypeDef *instance)
{
  if ((hspi == NULL) || (instance != SPI2))
  {
    return HAL_ERROR;
  }

  hspi->Instance = instance;
  hspi->Init.Mode = SPI_MODE_SLAVE;
  hspi->Init.Direction = SPI_DIRECTION_2LINES;
  hspi->Init.DataSize = SPI_DATASIZE_8BIT;
  hspi->Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi->Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi->Init.NSS = SPI_NSS_HARD_INPUT;
  hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi->Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi->Init.TIMode = SPI_TIMODE_DISABLE;
  hspi->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi->Init.CRCPolynomial = 10;

  return HAL_SPI_Init(hspi);
}

HAL_StatusTypeDef SPI_Slave_DeInit(SPI_HandleTypeDef *hspi)
{
  if ((hspi == NULL) || (hspi->Instance != SPI2))
  {
    return HAL_ERROR;
  }

  return HAL_SPI_DeInit(hspi);
}

HAL_StatusTypeDef SPI_Slave_Transmit(SPI_HandleTypeDef *hspi,
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

HAL_StatusTypeDef SPI_Slave_Receive(SPI_HandleTypeDef *hspi,
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

HAL_StatusTypeDef SPI_Slave_TransmitReceive(SPI_HandleTypeDef *hspi,
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
