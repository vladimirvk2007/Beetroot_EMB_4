#include "i2c/i2c.h"

HAL_StatusTypeDef I2C_Init(I2C_HandleTypeDef *hi2c,
                           I2C_TypeDef *instance,
                           uint32_t clock_speed)
{
  if ((hi2c == NULL) || (instance != I2C1) ||
      (clock_speed == 0) || (clock_speed > 400000U))
  {
    return HAL_ERROR;
  }

  hi2c->Instance = instance;
  hi2c->Init.ClockSpeed = clock_speed;
  hi2c->Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c->Init.OwnAddress1 = 0;
  hi2c->Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c->Init.OwnAddress2 = 0;
  hi2c->Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c->Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

  return HAL_I2C_Init(hi2c);
}

HAL_StatusTypeDef I2C_Transmit(I2C_HandleTypeDef *hi2c,
                               uint16_t device_address,
                               uint8_t *data,
                               uint16_t size,
                               uint32_t timeout)
{
  if ((hi2c == NULL) || (data == NULL) || (size == 0) ||
      (device_address > 0x7F))
  {
    return HAL_ERROR;
  }

  return HAL_I2C_Master_Transmit(hi2c, device_address << 1, data, size, timeout);
}

HAL_StatusTypeDef I2C_Receive(I2C_HandleTypeDef *hi2c,
                              uint16_t device_address,
                              uint8_t *data,
                              uint16_t size,
                              uint32_t timeout)
{
  if ((hi2c == NULL) || (data == NULL) || (size == 0) ||
      (device_address > 0x7F))
  {
    return HAL_ERROR;
  }

  return HAL_I2C_Master_Receive(hi2c, device_address << 1, data, size, timeout);
}

HAL_StatusTypeDef I2C_Probe(I2C_HandleTypeDef *hi2c,
                            uint16_t device_address,
                            uint32_t timeout)
{
  if ((hi2c == NULL) || (device_address > 0x7F))
  {
    return HAL_ERROR;
  }

  return HAL_I2C_IsDeviceReady(hi2c, device_address << 1, 1, timeout);
}

HAL_StatusTypeDef I2C_Scan(I2C_HandleTypeDef *hi2c,
                           uint8_t *found,
                           uint8_t max_found,
                           uint8_t *count,
                           uint32_t timeout)
{
  if ((hi2c == NULL) || (count == NULL) ||
      ((found == NULL) && (max_found != 0)))
  {
    return HAL_ERROR;
  }

  *count = 0;

  for (uint8_t address = 0x08; address <= 0x77; address++)
  {
    if (I2C_Probe(hi2c, address, timeout) == HAL_OK)
    {
      if (*count < max_found)
      {
        found[*count] = address;
      }
      (*count)++;
    }
  }

  return HAL_OK;
}

HAL_StatusTypeDef I2C_ReadRegister(I2C_HandleTypeDef *hi2c,
                                   uint16_t device_address,
                                   uint8_t reg,
                                   uint8_t *data,
                                   uint16_t size,
                                   uint32_t timeout)
{
  if ((hi2c == NULL) || (data == NULL) || (size == 0) ||
      (device_address > 0x7F))
  {
    return HAL_ERROR;
  }

  return HAL_I2C_Mem_Read(hi2c, device_address << 1, reg,
                          I2C_MEMADD_SIZE_8BIT, data, size, timeout);
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
  GPIO_InitTypeDef gpio_init = {0};

  if (hi2c->Instance != I2C1)
  {
    return;
  }

  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_I2C1_CLK_ENABLE();

  gpio_init.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  gpio_init.Mode = GPIO_MODE_AF_OD;
  gpio_init.Pull = GPIO_PULLUP;
  gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_init.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &gpio_init);
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance != I2C1)
  {
    return;
  }

  __HAL_RCC_I2C1_CLK_DISABLE();
  HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6 | GPIO_PIN_7);
}
