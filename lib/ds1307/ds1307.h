#ifndef DS1307_DRIVER_H
#define DS1307_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Непрозорий дескриптор DS1307. */
typedef struct ds1307_s *ds1307_handle_t;

/* Дата й час у локальному часі; рік підтримується в діапазоні 2000–2099. */
typedef struct {
    uint16_t year;
    uint8_t month;   /* 1–12 */
    uint8_t day;     /* 1–31 з урахуванням місяця */
    uint8_t weekday; /* 1 = неділя, ..., 7 = субота */
    uint8_t hour;    /* 0–23 */
    uint8_t minute;  /* 0–59 */
    uint8_t second;  /* 0–59 */
} ds1307_datetime_t;

/*
 * Додає DS1307 (I2C-адреса 0x68, 100 кГц) до ініціалізованої шини.
 * bus - дескриптор шини з i2c_bus_init()
 * rtc - [out] дескриптор створеного пристрою
 */
esp_err_t ds1307_init(i2c_master_bus_handle_t bus, ds1307_handle_t *rtc);

/* Видаляє пристрій DS1307; шина I2C залишається відкритою. */
esp_err_t ds1307_deinit(ds1307_handle_t rtc);

/* Читає дату й час; приймає як 12-, так і 24-годинний формат регістра годин. */
esp_err_t ds1307_get_datetime(ds1307_handle_t rtc, ds1307_datetime_t *datetime);

/*
 * Записує дату й час та запускає генератор часу.
 * weekday задається застосунком: 1 = неділя, ..., 7 = субота.
 */
esp_err_t ds1307_set_datetime(ds1307_handle_t rtc, const ds1307_datetime_t *datetime);

/* Перевіряє, чи працює генератор часу (біт CH регістра секунд). */
esp_err_t ds1307_is_running(ds1307_handle_t rtc, bool *running);

/* Зупиняє (running=false) або запускає (running=true) генератор часу. */
esp_err_t ds1307_set_running(ds1307_handle_t rtc, bool running);

#ifdef __cplusplus
}
#endif

#endif
