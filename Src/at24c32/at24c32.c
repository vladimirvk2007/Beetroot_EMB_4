#include <stdbool.h>
#include "at24c32/at24c32.h"

#define AT24C32_FILL_CHUNK 32U

static bool RangeIsValid(const AT24C32_HandleTypeDef *dev,
                         uint32_t address,
                         uint32_t size)
{
  return (address <= dev->size) && (size <= (dev->size - address));
}

static bool HandleIsValid(const AT24C32_HandleTypeDef *dev)
{
  return (dev != NULL) && (dev->hi2c != NULL) && (dev->page_size != 0U) &&
         (dev->size != 0U) && (dev->size <= 0x10000UL);
}

/* Після запису мікросхема не відповідає (NACK) до завершення внутрішнього циклу. */
static HAL_StatusTypeDef WaitWriteComplete(AT24C32_HandleTypeDef *dev)
{
  uint32_t start = HAL_GetTick();

  while (HAL_I2C_IsDeviceReady(dev->hi2c, (uint16_t)(dev->device_address << 1),
                               1, 1) != HAL_OK)
  {
    if ((HAL_GetTick() - start) > dev->write_cycle_timeout)
    {
      return HAL_TIMEOUT;
    }
  }

  return HAL_OK;
}

HAL_StatusTypeDef AT24C32_Init(AT24C32_HandleTypeDef *dev,
                               I2C_HandleTypeDef *hi2c,
                               uint8_t a_pins)
{
  if ((dev == NULL) || (hi2c == NULL) || (a_pins > 7U))
  {
    return HAL_ERROR;
  }

  dev->hi2c = hi2c;
  dev->device_address = (uint8_t)(AT24C32_BASE_ADDRESS | a_pins);
  dev->size = AT24C32_SIZE_BYTES;
  dev->page_size = AT24C32_PAGE_SIZE_BYTES;
  dev->timeout = AT24C32_DEFAULT_TIMEOUT_MS;
  dev->write_cycle_timeout = AT24C32_WRITE_CYCLE_MS;

  return AT24C32_IsReady(dev);
}

HAL_StatusTypeDef AT24C32_IsReady(AT24C32_HandleTypeDef *dev)
{
  if (!HandleIsValid(dev))
  {
    return HAL_ERROR;
  }

  return HAL_I2C_IsDeviceReady(dev->hi2c, (uint16_t)(dev->device_address << 1),
                               1, dev->timeout);
}

HAL_StatusTypeDef AT24C32_Read(AT24C32_HandleTypeDef *dev,
                               uint32_t address,
                               uint8_t *data,
                               uint32_t size)
{
  if (!HandleIsValid(dev) || (data == NULL) || (size == 0U) ||
      !RangeIsValid(dev, address, size))
  {
    return HAL_ERROR;
  }

  while (size > 0U)
  {
    uint16_t chunk = (size > 0xFFFFUL) ? 0xFFFFU : (uint16_t)size;

    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
        dev->hi2c, (uint16_t)(dev->device_address << 1), (uint16_t)address,
        I2C_MEMADD_SIZE_16BIT, data, chunk, dev->timeout);
    if (status != HAL_OK)
    {
      return status;
    }

    address += chunk;
    data += chunk;
    size -= chunk;
  }

  return HAL_OK;
}

HAL_StatusTypeDef AT24C32_Write(AT24C32_HandleTypeDef *dev,
                                uint32_t address,
                                const uint8_t *data,
                                uint32_t size)
{
  if (!HandleIsValid(dev) || (data == NULL) || (size == 0U) ||
      !RangeIsValid(dev, address, size))
  {
    return HAL_ERROR;
  }

  while (size > 0U)
  {
    uint32_t page_space = dev->page_size - (address % dev->page_size);
    uint16_t chunk = (uint16_t)((size < page_space) ? size : page_space);

    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(
        dev->hi2c, (uint16_t)(dev->device_address << 1), (uint16_t)address,
        I2C_MEMADD_SIZE_16BIT, (uint8_t *)data, chunk, dev->timeout);
    if (status != HAL_OK)
    {
      return status;
    }

    status = WaitWriteComplete(dev);
    if (status != HAL_OK)
    {
      return status;
    }

    address += chunk;
    data += chunk;
    size -= chunk;
  }

  return HAL_OK;
}

HAL_StatusTypeDef AT24C32_ReadByte(AT24C32_HandleTypeDef *dev,
                                   uint32_t address,
                                   uint8_t *value)
{
  return AT24C32_Read(dev, address, value, 1U);
}

HAL_StatusTypeDef AT24C32_WriteByte(AT24C32_HandleTypeDef *dev,
                                    uint32_t address,
                                    uint8_t value)
{
  return AT24C32_Write(dev, address, &value, 1U);
}

HAL_StatusTypeDef AT24C32_Fill(AT24C32_HandleTypeDef *dev,
                               uint32_t address,
                               uint32_t size,
                               uint8_t value)
{
  uint8_t buffer[AT24C32_FILL_CHUNK];

  if (!HandleIsValid(dev) || (size == 0U) || !RangeIsValid(dev, address, size))
  {
    return HAL_ERROR;
  }

  for (uint32_t i = 0; i < sizeof(buffer); i++)
  {
    buffer[i] = value;
  }

  while (size > 0U)
  {
    uint32_t chunk = (size < sizeof(buffer)) ? size : sizeof(buffer);

    HAL_StatusTypeDef status = AT24C32_Write(dev, address, buffer, chunk);
    if (status != HAL_OK)
    {
      return status;
    }

    address += chunk;
    size -= chunk;
  }

  return HAL_OK;
}
