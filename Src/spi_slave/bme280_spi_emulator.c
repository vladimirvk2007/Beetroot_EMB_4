#include "spi_slave/bme280_spi_emulator.h"

/* У команді SPI старший біт задає операцію, адреса займає 7 бітів. */
#define BME280_REG_INDEX(address) ((uint8_t)((address) & 0x7FU))
#define BME280_REG_RESET       BME280_REG_INDEX(0xE0U)
#define BME280_REG_CTRL_HUM    BME280_REG_INDEX(0xF2U)
#define BME280_REG_STATUS      BME280_REG_INDEX(0xF3U)
#define BME280_REG_CTRL_MEAS   BME280_REG_INDEX(0xF4U)
#define BME280_REG_CONFIG      BME280_REG_INDEX(0xF5U)
#define BME280_RESET_COMMAND   0xB6U

#define BME280_TRANSACTION_IDLE 0U
#define BME280_TRANSACTION_ACTIVE 1U

static BME280_SPI_Emulator_HandleTypeDef * volatile active_emulator;

/* Завантажує калібрування, ID та тестові дані вимірювань. */
static void BME280_SPI_Emulator_LoadDefaults(
    BME280_SPI_Emulator_HandleTypeDef *emulator)
{
  static const uint8_t calibration_temperature_pressure[] = {
    0x70, 0x6B, 0x43, 0x67, 0x18, 0xFC,
    0x7D, 0x8E, 0x43, 0xD6, 0xD0, 0x0B,
    0x27, 0x0B, 0x8C, 0x00, 0xF9, 0xFF,
    0x8C, 0x3C, 0xF8, 0xC6, 0x70, 0x17
  };
  static const uint8_t calibration_humidity[] = {
    0x6A, 0x01, 0x00, 0x14, 0x2E, 0x03, 0x1E
  };

  for (uint16_t i = 0; i < BME280_SPI_EMULATOR_REGISTER_COUNT; i++)
  {
    emulator->registers[i] = 0;
  }

  for (uint8_t i = 0; i < sizeof(calibration_temperature_pressure); i++)
  {
    emulator->registers[BME280_REG_INDEX(0x88U + i)] =
        calibration_temperature_pressure[i];
  }

  emulator->registers[BME280_REG_INDEX(0xA1U)] = 75U;
  for (uint8_t i = 0; i < sizeof(calibration_humidity); i++)
  {
    emulator->registers[BME280_REG_INDEX(0xE1U + i)] =
        calibration_humidity[i];
  }

  emulator->registers[BME280_REG_INDEX(
      BME280_SPI_EMULATOR_CHIP_ID_REGISTER)] =
      BME280_SPI_EMULATOR_CHIP_ID;
  emulator->registers[BME280_REG_STATUS] = 0U;
  emulator->registers[BME280_REG_CTRL_HUM] = 0U;
  emulator->registers[BME280_REG_CTRL_MEAS] = 0U;
  emulator->registers[BME280_REG_CONFIG] = 0U;

  /* Фіксовані тестові дані, а не поточне вимірювання. */
  emulator->registers[BME280_REG_INDEX(0xF7U)] = 0x65;
  emulator->registers[BME280_REG_INDEX(0xF8U)] = 0x5A;
  emulator->registers[BME280_REG_INDEX(0xF9U)] = 0xC0;
  emulator->registers[BME280_REG_INDEX(0xFAU)] = 0x7E;
  emulator->registers[BME280_REG_INDEX(0xFBU)] = 0xED;
  emulator->registers[BME280_REG_INDEX(0xFCU)] = 0x00;
  emulator->registers[BME280_REG_INDEX(0xFDU)] = 0x7E;
  emulator->registers[BME280_REG_INDEX(0xFEU)] = 0x00;
}

static HAL_StatusTypeDef BME280_SPI_Emulator_Arm(
    BME280_SPI_Emulator_HandleTypeDef *emulator)
{
  /* Приймаємо й передаємо по одному байту через переривання. */
  return HAL_SPI_TransmitReceive_IT(emulator->hspi,
                                    (uint8_t *)&emulator->tx_byte,
                                    (uint8_t *)&emulator->rx_byte,
                                    1U);
}

static void BME280_SPI_Emulator_ProcessByte(
    BME280_SPI_Emulator_HandleTypeDef *emulator,
    uint8_t received)
{
  /* Перший байт транзакції містить напрямок і адресу регістра. */
  if (emulator->command_received == BME280_TRANSACTION_IDLE)
  {
    emulator->is_read = (received & 0x80U) != 0U;
    emulator->address = received & 0x7FU;
    emulator->command_received = BME280_TRANSACTION_ACTIVE;
    emulator->tx_byte = emulator->is_read
                            ? emulator->registers[emulator->address]
                            : 0U;
    return;
  }

  if (emulator->is_read != 0U)
  {
    /* Після кожного байта читання переходимо до наступного регістра. */
    emulator->address = (emulator->address + 1U) & 0x7FU;
    emulator->tx_byte = emulator->registers[emulator->address];
    return;
  }

  if (emulator->address == BME280_REG_RESET)
  {
    if (received == BME280_RESET_COMMAND)
    {
      BME280_SPI_Emulator_LoadDefaults(emulator);
    }
  }
  else if ((emulator->address == BME280_REG_CTRL_HUM) ||
           (emulator->address == BME280_REG_CTRL_MEAS) ||
           (emulator->address == BME280_REG_CONFIG))
  {
    emulator->registers[emulator->address] = received;
  }

  emulator->address = (emulator->address + 1U) & 0x7FU;
  emulator->tx_byte = 0U;
}

HAL_StatusTypeDef BME280_SPI_Emulator_Init(
    BME280_SPI_Emulator_HandleTypeDef *emulator,
    SPI_HandleTypeDef *hspi)
{
  if ((emulator == NULL) || (hspi == NULL) ||
      (hspi->Instance != SPI2) || (hspi->State != HAL_SPI_STATE_READY))
  {
    return HAL_ERROR;
  }

  emulator->hspi = hspi;
  emulator->address = 0U;
  emulator->is_read = 0U;
  emulator->command_received = BME280_TRANSACTION_IDLE;
  emulator->tx_byte = 0U;
  emulator->rx_byte = 0U;
  emulator->last_status = HAL_OK;
  BME280_SPI_Emulator_LoadDefaults(emulator);

  return HAL_OK;
}

HAL_StatusTypeDef BME280_SPI_Emulator_Start(
    BME280_SPI_Emulator_HandleTypeDef *emulator)
{
  if ((emulator == NULL) || (emulator->hspi == NULL) ||
      (emulator->hspi->Instance != SPI2) ||
      (emulator->hspi->State != HAL_SPI_STATE_READY) ||
      ((active_emulator != NULL) && (active_emulator != emulator)))
  {
    return HAL_ERROR;
  }

  emulator->address = 0U;
  emulator->is_read = 0U;
  emulator->command_received = BME280_TRANSACTION_IDLE;
  emulator->tx_byte = 0U;
  emulator->last_status = HAL_OK;
  active_emulator = emulator;

  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* NSS на PB12 скидає стан команди на початку кожної транзакції. */
  SYSCFG->EXTICR[3] = (SYSCFG->EXTICR[3] & ~0x0FU) | 0x01U;
  EXTI->IMR |= GPIO_PIN_12;
  EXTI->RTSR |= GPIO_PIN_12;
  EXTI->FTSR |= GPIO_PIN_12;
  EXTI->PR = GPIO_PIN_12;

  HAL_NVIC_SetPriority(SPI2_IRQn, 5U, 0U);
  HAL_NVIC_EnableIRQ(SPI2_IRQn);
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5U, 0U);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  emulator->last_status = BME280_SPI_Emulator_Arm(emulator);
  if (emulator->last_status != HAL_OK)
  {
    EXTI->IMR &= ~GPIO_PIN_12;
    active_emulator = NULL;
    return emulator->last_status;
  }

  return HAL_OK;
}

HAL_StatusTypeDef BME280_SPI_Emulator_Stop(
    BME280_SPI_Emulator_HandleTypeDef *emulator)
{
  HAL_StatusTypeDef status;

  if ((emulator == NULL) || (active_emulator != emulator))
  {
    return HAL_ERROR;
  }

  EXTI->IMR &= ~GPIO_PIN_12;
  EXTI->PR = GPIO_PIN_12;
  status = HAL_SPI_Abort(emulator->hspi);
  emulator->last_status = status;
  active_emulator = NULL;

  return status;
}

HAL_StatusTypeDef BME280_SPI_Emulator_GetRegister(
    BME280_SPI_Emulator_HandleTypeDef *emulator,
    uint8_t address,
    uint8_t *value)
{
  if ((emulator == NULL) || (emulator->hspi == NULL) || (value == NULL))
  {
    return HAL_ERROR;
  }

  *value = emulator->registers[BME280_REG_INDEX(address)];
  return HAL_OK;
}

HAL_StatusTypeDef BME280_SPI_Emulator_SetRegister(
    BME280_SPI_Emulator_HandleTypeDef *emulator,
    uint8_t address,
    uint8_t value)
{
  if ((emulator == NULL) || (emulator->hspi == NULL))
  {
    return HAL_ERROR;
  }

  emulator->registers[BME280_REG_INDEX(address)] = value;
  return HAL_OK;
}

HAL_StatusTypeDef BME280_SPI_Emulator_GetStatus(
    BME280_SPI_Emulator_HandleTypeDef *emulator)
{
  if ((emulator == NULL) || (emulator->hspi == NULL))
  {
    return HAL_ERROR;
  }

  return emulator->last_status;
}

void BME280_SPI_Emulator_IRQHandler(void)
{
  if ((active_emulator != NULL) && (active_emulator->hspi != NULL))
  {
    HAL_SPI_IRQHandler(active_emulator->hspi);
  }
}

void BME280_SPI_Emulator_NssChanged(void)
{
  if (active_emulator != NULL)
  {
    active_emulator->command_received = BME280_TRANSACTION_IDLE;
    active_emulator->address = 0U;
    active_emulator->is_read = 0U;
  }
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if ((active_emulator == NULL) || (active_emulator->hspi != hspi))
  {
    return;
  }

  BME280_SPI_Emulator_ProcessByte(active_emulator, active_emulator->rx_byte);
  active_emulator->last_status = BME280_SPI_Emulator_Arm(active_emulator);
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
  if ((active_emulator != NULL) && (active_emulator->hspi == hspi))
  {
    active_emulator->last_status = HAL_ERROR;
  }
}
