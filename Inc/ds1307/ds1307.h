#ifndef DS1307_H
#define DS1307_H

#include <stdbool.h>
#include <stdint.h>

#include "i2c/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Непрозорий дескриптор DS1307. */
typedef struct DS1307_HandleTypeDef DS1307_HandleTypeDef;

/* Дата й час; рік підтримується в діапазоні 2000-2099. */
typedef struct
{
  uint16_t year;
  uint8_t month;   /* 1-12 */
  uint8_t day;     /* 1-31 з урахуванням місяця */
  uint8_t weekday; /* 1 = неділя, ..., 7 = субота */
  uint8_t hour;    /* 0-23 */
  uint8_t minute;  /* 0-59 */
  uint8_t second;  /* 0-59 */
} DS1307_DateTimeTypeDef;

/*
 * Додає DS1307 (I2C-адреса 0x68) до ініціалізованої шини.
 * hi2c - дескриптор шини (після I2C_Init)
 * rtc  - [out] дескриптор створеного пристрою
 */
HAL_StatusTypeDef DS1307_Init(I2C_HandleTypeDef *hi2c, DS1307_HandleTypeDef **rtc);

/* Видаляє пристрій DS1307; шина I2C залишається відкритою. */
HAL_StatusTypeDef DS1307_Deinit(DS1307_HandleTypeDef *rtc);

/* Читає дату й час; приймає 12- і 24-годинний формат регістра годин. */
HAL_StatusTypeDef DS1307_GetDateTime(DS1307_HandleTypeDef *rtc, DS1307_DateTimeTypeDef *datetime);

/*
 * Записує дату й час та запускає генератор часу.
 * weekday задається застосунком: 1 = неділя, ..., 7 = субота.
 */
HAL_StatusTypeDef DS1307_SetDateTime(DS1307_HandleTypeDef *rtc, const DS1307_DateTimeTypeDef *datetime);

/* Перевіряє, чи працює генератор часу (біт CH регістра секунд). */
HAL_StatusTypeDef DS1307_IsRunning(DS1307_HandleTypeDef *rtc, bool *running);

/* Зупиняє (running = false) або запускає (running = true) генератор часу. */
HAL_StatusTypeDef DS1307_SetRunning(DS1307_HandleTypeDef *rtc, bool running);

#ifdef __cplusplus
}
#endif

#endif /* DS1307_H */
