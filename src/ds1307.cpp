#include "ds1307.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static uint8_t bcd_to_dec(uint8_t bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

static uint8_t dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

esp_err_t ds1307_read_time(i2c_port_t port, uint8_t address, SemaphoreHandle_t i2c_mutex, struct RtcDateTime* out) {
    if (out == nullptr || i2c_mutex == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t reg = 0x00;
    uint8_t raw[7] = {0};
    esp_err_t err = ESP_FAIL;

    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    for (int attempt = 0; attempt < 3; ++attempt) {
        err = i2c_master_write_to_device(
            port,
            address,
            &reg,
            1,
            pdMS_TO_TICKS(200));
        if (err == ESP_OK) {
            err = i2c_master_read_from_device(
                port,
                address,
                raw,
                sizeof(raw),
                pdMS_TO_TICKS(200));
        }
        if (err == ESP_OK) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    xSemaphoreGive(i2c_mutex);

    if (err != ESP_OK) {
        return err;
    }

    out->second = bcd_to_dec(raw[0] & 0x7F);
    out->minute = bcd_to_dec(raw[1] & 0x7F);
    out->hour = bcd_to_dec(raw[2] & 0x3F);
    out->day = bcd_to_dec(raw[4] & 0x3F);
    out->month = bcd_to_dec(raw[5] & 0x1F);
    out->year = 2000 + bcd_to_dec(raw[6]);

    return ESP_OK;
}

esp_err_t ds1307_set_time(i2c_port_t port,
                          uint8_t address,
                          SemaphoreHandle_t i2c_mutex,
                          const struct RtcDateTime* in) {
    if (in == nullptr || i2c_mutex == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    if ((in->year < 2000) ||
        (in->year > 2099) ||
        (in->month < 1) ||
        (in->month > 12) ||
        (in->day < 1) ||
        (in->day > 31) ||
        (in->hour > 23) ||
        (in->minute > 59) ||
        (in->second > 59)) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t payload[8] = {
        0x00,
        dec_to_bcd(in->second),
        dec_to_bcd(in->minute),
        dec_to_bcd(in->hour),
        0x01,
        dec_to_bcd(in->day),
        dec_to_bcd(in->month),
        dec_to_bcd(in->year - 2000),
    };

    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t err = i2c_master_write_to_device(
        port,
        address,
        payload,
        sizeof(payload),
        pdMS_TO_TICKS(200));

    xSemaphoreGive(i2c_mutex);
    return err;
}
