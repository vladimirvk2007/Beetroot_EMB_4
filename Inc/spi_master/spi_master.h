#ifndef SPI_MASTER_H
#define SPI_MASTER_H

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

HAL_StatusTypeDef SPI_Master_Init(SPI_HandleTypeDef *hspi,
                                 SPI_TypeDef *instance,
                                 uint32_t baud_prescaler);
HAL_StatusTypeDef SPI_Master_DeInit(SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef SPI_Master_Transmit(SPI_HandleTypeDef *hspi,
                                      uint8_t *data,
                                      uint16_t size,
                                      uint32_t timeout);
HAL_StatusTypeDef SPI_Master_Receive(SPI_HandleTypeDef *hspi,
                                     uint8_t *data,
                                     uint16_t size,
                                     uint32_t timeout);
HAL_StatusTypeDef SPI_Master_TransmitReceive(SPI_HandleTypeDef *hspi,
                                             uint8_t *tx_data,
                                             uint8_t *rx_data,
                                             uint16_t size,
                                             uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* SPI_MASTER_H */
