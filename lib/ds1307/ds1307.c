#include "ds1307.h"

#include <stdbool.h>
#include <stdlib.h>

#include "esp_log.h"
#include "i2c.h"

#define DS1307_I2C_ADDRESS 0x68U
#define DS1307_I2C_SPEED_HZ 100000U
#define DS1307_I2C_TIMEOUT_MS 1000U
#define DS1307_REG_SECONDS 0x00U
#define DS1307_REG_DATETIME 0x00U
#define DS1307_DATETIME_REGISTER_COUNT 7U
#define DS1307_CH_BIT 0x80U

static const char *TAG = "ds1307";

struct ds1307_s {
    i2c_master_dev_handle_t device;
};

static uint8_t to_bcd(uint8_t value)
{
    return (uint8_t)(((value / 10U) << 4U) | (value % 10U));
}

static bool from_bcd(uint8_t value, uint8_t *decoded)
{
    const uint8_t tens = (uint8_t)((value >> 4U) & 0x0FU);
    const uint8_t ones = (uint8_t)(value & 0x0FU);
    if (tens > 9U || ones > 9U) {
        return false;
    }
    *decoded = (uint8_t)(tens * 10U + ones);
    return true;
}

static bool is_leap_year(uint16_t year)
{
    return (year % 4U) == 0U;
}

static uint8_t days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days[] = {31U, 28U, 31U, 30U, 31U, 30U,
                                   31U, 31U, 30U, 31U, 30U, 31U};
    if (month == 2U && is_leap_year(year)) {
        return 29U;
    }
    return days[month - 1U];
}

static bool datetime_is_valid(const ds1307_datetime_t *datetime)
{
    if (datetime == NULL || datetime->year < 2000U || datetime->year > 2099U ||
        datetime->month < 1U || datetime->month > 12U ||
        datetime->weekday < 1U || datetime->weekday > 7U ||
        datetime->hour > 23U || datetime->minute > 59U || datetime->second > 59U) {
        return false;
    }
    return datetime->day >= 1U && datetime->day <= days_in_month(datetime->year, datetime->month);
}

esp_err_t ds1307_init(i2c_master_bus_handle_t bus, ds1307_handle_t *rtc)
{
    if (bus == NULL || rtc == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *rtc = NULL;

    struct ds1307_s *ctx = calloc(1U, sizeof(*ctx));
    if (ctx == NULL) {
        return ESP_ERR_NO_MEM;
    }

    const i2c_device_settings_t settings = {
        .address = DS1307_I2C_ADDRESS,
        .scl_speed_hz = DS1307_I2C_SPEED_HZ,
    };
    const esp_err_t err = i2c_device_add(bus, &settings, &ctx->device);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "DS1307 initialization failed: %s", esp_err_to_name(err));
        free(ctx);
        return err;
    }

    *rtc = ctx;
    return ESP_OK;
}

esp_err_t ds1307_deinit(ds1307_handle_t rtc)
{
    if (rtc == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_err_t err = i2c_device_remove(rtc->device);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "DS1307 deinitialization failed: %s", esp_err_to_name(err));
        return err;
    }
    free(rtc);
    return ESP_OK;
}

esp_err_t ds1307_get_datetime(ds1307_handle_t rtc, ds1307_datetime_t *datetime)
{
    if (rtc == NULL || datetime == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t registers[DS1307_DATETIME_REGISTER_COUNT];
    esp_err_t err = i2c_read_register(rtc->device, DS1307_REG_DATETIME,
                                      registers, sizeof(registers), DS1307_I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read date/time: %s", esp_err_to_name(err));
        return err;
    }

    ds1307_datetime_t value = {0};
    if (!from_bcd(registers[0] & 0x7FU, &value.second) ||
        !from_bcd(registers[1] & 0x7FU, &value.minute)) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    const uint8_t hour_register = registers[2];
    if ((hour_register & 0x40U) != 0U) {
        uint8_t hour_12;
        if (!from_bcd(hour_register & 0x1FU, &hour_12) || hour_12 < 1U || hour_12 > 12U) {
            return ESP_ERR_INVALID_RESPONSE;
        }
        const bool is_pm = (hour_register & 0x20U) != 0U;
        value.hour = (uint8_t)((hour_12 % 12U) + (is_pm ? 12U : 0U));
    } else if (!from_bcd(hour_register & 0x3FU, &value.hour)) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    if (!from_bcd(registers[3] & 0x07U, &value.weekday) ||
        !from_bcd(registers[4] & 0x3FU, &value.day) ||
        !from_bcd(registers[5] & 0x1FU, &value.month)) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    uint8_t year;
    if (!from_bcd(registers[6], &year)) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    value.year = (uint16_t)(2000U + year);

    if (!datetime_is_valid(&value)) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    *datetime = value;
    return ESP_OK;
}

esp_err_t ds1307_set_datetime(ds1307_handle_t rtc, const ds1307_datetime_t *datetime)
{
    if (rtc == NULL || !datetime_is_valid(datetime)) {
        return ESP_ERR_INVALID_ARG;
    }

    const uint8_t registers[DS1307_DATETIME_REGISTER_COUNT] = {
        to_bcd(datetime->second),
        to_bcd(datetime->minute),
        to_bcd(datetime->hour),
        to_bcd(datetime->weekday),
        to_bcd(datetime->day),
        to_bcd(datetime->month),
        to_bcd((uint8_t)(datetime->year - 2000U)),
    };
    const esp_err_t err = i2c_write_register(rtc->device, DS1307_REG_DATETIME,
                                             registers, sizeof(registers), DS1307_I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write date/time: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t ds1307_is_running(ds1307_handle_t rtc, bool *running)
{
    if (rtc == NULL || running == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t seconds;
    const esp_err_t err = i2c_read_register(rtc->device, DS1307_REG_SECONDS,
                                            &seconds, sizeof(seconds), DS1307_I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read oscillator state: %s", esp_err_to_name(err));
        return err;
    }
    *running = (seconds & DS1307_CH_BIT) == 0U;
    return ESP_OK;
}

esp_err_t ds1307_set_running(ds1307_handle_t rtc, bool running)
{
    if (rtc == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t seconds;
    esp_err_t err = i2c_read_register(rtc->device, DS1307_REG_SECONDS,
                                      &seconds, sizeof(seconds), DS1307_I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read seconds register: %s", esp_err_to_name(err));
        return err;
    }

    seconds = running ? (uint8_t)(seconds & (uint8_t)~DS1307_CH_BIT)
                      : (uint8_t)(seconds | DS1307_CH_BIT);
    err = i2c_write_register(rtc->device, DS1307_REG_SECONDS,
                             &seconds, sizeof(seconds), DS1307_I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update oscillator state: %s", esp_err_to_name(err));
    }
    return err;
}