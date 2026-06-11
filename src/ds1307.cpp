#include "ds1307.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static uint8_t bcd_to_dec(uint8_t bcd) {
    return static_cast<uint8_t>(((bcd >> 4U) * 10U) + (bcd & 0x0FU));
}

esp_err_t ds1307_read_time(i2c_port_t port, uint8_t address, SemaphoreHandle_t i2c_mutex, struct RtcDateTime* out) {
    if ((out == nullptr) || (i2c_mutex == nullptr)) {
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
    out->year = static_cast<uint16_t>(2000U + bcd_to_dec(raw[6]));

    return ESP_OK;
}
