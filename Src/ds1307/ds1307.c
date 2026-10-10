#include "ds1307/ds1307.h"

#include <stdlib.h>

#define DS1307_ADDRESS 0x68U
#define DS1307_TIMEOUT_MS 100U
#define DS1307_REG_SECONDS 0x00U
#define DS1307_REG_COUNT 7U

#define DS1307_CH_BIT 0x80U  /* Clock Halt у регістрі секунд */
#define DS1307_12H_BIT 0x40U /* 12-годинний режим у регістрі годин */
#define DS1307_PM_BIT 0x20U  /* PM у 12-годинному режимі */

/* Внутрішній стан пристрою. */
struct DS1307_HandleTypeDef
{
  I2C_HandleTypeDef *i2c;
};

/* Число -> BCD. */
static uint8_t DS1307_ToBcd(uint8_t value)
{
  return (uint8_t)(((value / 10U) << 4) | (value % 10U));
}

/* BCD -> число. */
static uint8_t DS1307_FromBcd(uint8_t value)
{
  return (uint8_t)(((value >> 4) * 10U) + (value & 0x0FU));
}

/* Перевірка високосного року. */
static bool DS1307_IsLeapYear(uint16_t year)
{
  return ((year % 4U) == 0U) && (((year % 100U) != 0U) || ((year % 400U) == 0U));
}

/* Кількість днів у місяці. */
static uint8_t DS1307_DaysInMonth(uint16_t year, uint8_t month)
{
  static const uint8_t days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  if ((month == 2U) && DS1307_IsLeapYear(year))
  {
    return 29U;
  }
  return days[month - 1U];
}

/* Запис регістрів одним пакетом [reg, data...]. */
static HAL_StatusTypeDef DS1307_WriteRegisters(const DS1307_HandleTypeDef *rtc,
                                               uint8_t reg,
                                               const uint8_t *data,
                                               uint8_t size)
{
  uint8_t packet[DS1307_REG_COUNT + 1U];

  packet[0] = reg;
  for (uint8_t i = 0U; i < size; i++)
  {
    packet[i + 1U] = data[i];
  }
  return I2C_Transmit(rtc->i2c, DS1307_ADDRESS, packet, (uint16_t)(size + 1U),
                      DS1307_TIMEOUT_MS);
}

/* Читання регістрів, починаючи з reg. */
static HAL_StatusTypeDef DS1307_ReadRegisters(const DS1307_HandleTypeDef *rtc,
                                              uint8_t reg,
                                              uint8_t *data,
                                              uint8_t size)
{
  return I2C_ReadRegister(rtc->i2c, DS1307_ADDRESS, reg, data, size,
                          DS1307_TIMEOUT_MS);
}

/* Перевіряє наявність годинника на шині та створює дескриптор. */
HAL_StatusTypeDef DS1307_Init(I2C_HandleTypeDef *hi2c, DS1307_HandleTypeDef **rtc)
{
  if ((hi2c == NULL) || (rtc == NULL))
  {
    return HAL_ERROR;
  }

  /* Перевіряємо наявність годинника на шині */
  HAL_StatusTypeDef status = I2C_Probe(hi2c, DS1307_ADDRESS, DS1307_TIMEOUT_MS);
  if (status != HAL_OK)
  {
    return status;
  }

  DS1307_HandleTypeDef *dev = calloc(1U, sizeof(*dev));
  if (dev == NULL)
  {
    return HAL_ERROR;
  }

  dev->i2c = hi2c;
  *rtc = dev;
  return HAL_OK;
}

/* Звільняє дескриптор; шина I2C не закривається. */
HAL_StatusTypeDef DS1307_Deinit(DS1307_HandleTypeDef *rtc)
{
  if (rtc == NULL)
  {
    return HAL_ERROR;
  }

  free(rtc);
  return HAL_OK;
}

/* Читає дату й час із регістрів. */
HAL_StatusTypeDef DS1307_GetDateTime(DS1307_HandleTypeDef *rtc, DS1307_DateTimeTypeDef *datetime)
{
  uint8_t regs[DS1307_REG_COUNT];

  if ((rtc == NULL) || (datetime == NULL))
  {
    return HAL_ERROR;
  }

  HAL_StatusTypeDef status =
      DS1307_ReadRegisters(rtc, DS1307_REG_SECONDS, regs, DS1307_REG_COUNT);
  if (status != HAL_OK)
  {
    return status;
  }

  /* Приводимо 12-годинний формат до 24-годинного */
  uint8_t hour;
  if ((regs[2] & DS1307_12H_BIT) != 0U)
  {
    hour = (uint8_t)(DS1307_FromBcd(regs[2] & 0x1FU) % 12U);
    if ((regs[2] & DS1307_PM_BIT) != 0U)
    {
      hour += 12U;
    }
  }
  else
  {
    hour = DS1307_FromBcd(regs[2] & 0x3FU);
  }

  datetime->second = DS1307_FromBcd(regs[0] & (uint8_t)~DS1307_CH_BIT);
  datetime->minute = DS1307_FromBcd(regs[1]);
  datetime->hour = hour;
  datetime->weekday = DS1307_FromBcd(regs[3]);
  datetime->day = DS1307_FromBcd(regs[4]);
  datetime->month = DS1307_FromBcd(regs[5]);
  datetime->year = (uint16_t)(2000U + DS1307_FromBcd(regs[6]));
  return HAL_OK;
}

/* Перевіряє й записує дату та час, запускає генератор. */
HAL_StatusTypeDef DS1307_SetDateTime(DS1307_HandleTypeDef *rtc, const DS1307_DateTimeTypeDef *datetime)
{
  /* Перевірка допустимості дати й часу */
  if ((rtc == NULL) || (datetime == NULL) ||
      (datetime->year < 2000U) || (datetime->year > 2099U) ||
      (datetime->month < 1U) || (datetime->month > 12U) ||
      (datetime->day < 1U) ||
      (datetime->day > DS1307_DaysInMonth(datetime->year, datetime->month)) ||
      (datetime->weekday < 1U) || (datetime->weekday > 7U) ||
      (datetime->hour > 23U) || (datetime->minute > 59U) ||
      (datetime->second > 59U))
  {
    return HAL_ERROR;
  }

  /* Біт CH = 0 у секундах запускає генератор */
  const uint8_t regs[DS1307_REG_COUNT] = {
      DS1307_ToBcd(datetime->second),
      DS1307_ToBcd(datetime->minute),
      DS1307_ToBcd(datetime->hour),
      DS1307_ToBcd(datetime->weekday),
      DS1307_ToBcd(datetime->day),
      DS1307_ToBcd(datetime->month),
      DS1307_ToBcd((uint8_t)(datetime->year - 2000U)),
  };

  return DS1307_WriteRegisters(rtc, DS1307_REG_SECONDS, regs, DS1307_REG_COUNT);
}

/* Повертає стан генератора (біт CH). */
HAL_StatusTypeDef DS1307_IsRunning(DS1307_HandleTypeDef *rtc, bool *running)
{
  uint8_t seconds;

  if ((rtc == NULL) || (running == NULL))
  {
    return HAL_ERROR;
  }

  HAL_StatusTypeDef status =
      DS1307_ReadRegisters(rtc, DS1307_REG_SECONDS, &seconds, 1U);
  if (status != HAL_OK)
  {
    return status;
  }

  *running = (seconds & DS1307_CH_BIT) == 0U;
  return HAL_OK;
}

/* Запускає або зупиняє генератор, не змінюючи секунди. */
HAL_StatusTypeDef DS1307_SetRunning(DS1307_HandleTypeDef *rtc, bool running)
{
  uint8_t seconds;

  if (rtc == NULL)
  {
    return HAL_ERROR;
  }

  HAL_StatusTypeDef status =
      DS1307_ReadRegisters(rtc, DS1307_REG_SECONDS, &seconds, 1U);
  if (status != HAL_OK)
  {
    return status;
  }

  if (running)
  {
    seconds &= (uint8_t)~DS1307_CH_BIT;
  }
  else
  {
    seconds |= DS1307_CH_BIT;
  }

  return DS1307_WriteRegisters(rtc, DS1307_REG_SECONDS, &seconds, 1U);
}
