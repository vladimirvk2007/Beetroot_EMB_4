#include "bme280_i2c.h"

#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

constexpr uint8_t BME280_CHIP_ID_REG = 0xD0;
constexpr uint8_t BME280_RESET_REG = 0xE0;
constexpr uint8_t BME280_STATUS_REG = 0xF3;
constexpr uint8_t BME280_CTRL_HUM_REG = 0xF2;
constexpr uint8_t BME280_CTRL_MEAS_REG = 0xF4;
constexpr uint8_t BME280_PRESS_MSB_REG = 0xF7;
constexpr uint8_t BME280_CHIP_ID = 0x60;

esp_err_t bme280_write_register(i2c_port_t port,
                                uint8_t address,
                                SemaphoreHandle_t i2c_mutex,
                                uint8_t reg,
                                uint8_t value)
{
    if (i2c_mutex == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t payload[2] = {reg, value};

    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t err = i2c_master_write_to_device(port,
                                               address,
                                               payload,
                                               sizeof(payload),
                                               pdMS_TO_TICKS(200));
    xSemaphoreGive(i2c_mutex);
    return err;
}

esp_err_t bme280_read_registers(i2c_port_t port,
                                uint8_t address,
                                SemaphoreHandle_t i2c_mutex,
                                uint8_t reg,
                                uint8_t *data,
                                size_t size)
{
    if ((data == nullptr) || (size == 0U) || (i2c_mutex == nullptr)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t err = i2c_master_write_read_device(port,
                                                 address,
                                                 &reg,
                                                 1,
                                                 data,
                                                 size,
                                                 pdMS_TO_TICKS(200));
    xSemaphoreGive(i2c_mutex);
    return err;
}

uint16_t pressure_hpa_to_mmhg(float pressure_hpa)
{
    const float pressure_mmhg = pressure_hpa * 0.75006156f;
    if (pressure_mmhg <= 0.0f) {
        return 0;
    }

    return static_cast<uint16_t>(lroundf(pressure_mmhg));
}

} // namespace

static uint16_t u16_le(const uint8_t *d)
{
    return (uint16_t)d[0] | ((uint16_t)d[1] << 8);
}

static int16_t s16_le(const uint8_t *d)
{
    return (int16_t)u16_le(d);
}

static int32_t bme280_compensate_temperature(int32_t adc_T, const Bme280Calib *cal)
{
    // Повертає t_fine для наступної компенсації тиску/вологості.
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)cal->dig_T1 << 1))) * ((int32_t)cal->dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)cal->dig_T1)) *
                      ((adc_T >> 4) - ((int32_t)cal->dig_T1))) >> 12) *
                    ((int32_t)cal->dig_T3)) >> 14;
    return var1 + var2;
}

static float bme280_temperature_c_from_tfine(int32_t t_fine)
{
    return (float)((t_fine * 5 + 128) >> 8) / 100.0f;
}

static float bme280_compensate_pressure(int32_t adc_P, int32_t t_fine, const Bme280Calib *cal)
{
    // Фіксована арифметика з Bosch reference code.
    int64_t var1 = ((int64_t)t_fine) - 128000;
    int64_t var2 = var1 * var1 * (int64_t)cal->dig_P6;
    var2 = var2 + ((var1 * (int64_t)cal->dig_P5) << 17);
    var2 = var2 + (((int64_t)cal->dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)cal->dig_P3) >> 8) + ((var1 * (int64_t)cal->dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1) * ((int64_t)cal->dig_P1)) >> 33;

    if (var1 == 0) {
        return 0.0f;
    }

    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)cal->dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)cal->dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)cal->dig_P7) << 4);

    // Формат Q24.8 у Па -> hPa.
    return ((float)p / 256.0f) / 100.0f;
}

static float bme280_compensate_humidity(int32_t adc_H, int32_t t_fine, const Bme280Calib *cal)
{
    // Floating-point компенсація для вологості за Bosch reference.
    float var_H = ((float)t_fine) - 76800.0f;
    var_H = (adc_H - (((float)cal->dig_H4) * 64.0f + ((float)cal->dig_H5) / 16384.0f * var_H)) *
            (((float)cal->dig_H2) / 65536.0f *
             (1.0f + ((float)cal->dig_H6) / 67108864.0f * var_H *
              (1.0f + ((float)cal->dig_H3) / 67108864.0f * var_H)));
    var_H = var_H * (1.0f - ((float)cal->dig_H1) * var_H / 524288.0f);

    if (var_H > 100.0f) {
        var_H = 100.0f;
    } else if (var_H < 0.0f) {
        var_H = 0.0f;
    }

    return var_H;
}

esp_err_t bme280_init(i2c_port_t port, uint8_t address, SemaphoreHandle_t i2c_mutex, Bme280Calib *cal)
{
    if ((cal == nullptr) || (i2c_mutex == nullptr)) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t chip_id = 0;
    esp_err_t err = bme280_read_registers(port, address, i2c_mutex, BME280_CHIP_ID_REG, &chip_id, 1);
    if (err != ESP_OK) {
        return err;
    }

    if (chip_id != BME280_CHIP_ID) {
        return ESP_ERR_NOT_FOUND;
    }

    err = bme280_write_register(port, address, i2c_mutex, BME280_RESET_REG, 0xB6);
    if (err != ESP_OK) {
        return err;
    }

    // After soft reset the chip reloads calibration from NVM and sets the
    // im_update bit (bit 0 of status register 0xF3) while doing so.
    // Reading calibration before im_update clears produces zeros for the T/P
    // block (0x88), while the H block (0xE1) may still read correctly.
    // Poll with a 2 ms interval and allow up to 100 ms total.
    for (int attempt = 0; attempt < 50; ++attempt) {
        vTaskDelay(pdMS_TO_TICKS(2));
        uint8_t status = 0;
        err = bme280_read_registers(port, address, i2c_mutex, BME280_STATUS_REG, &status, 1);
        if (err == ESP_OK && (status & 0x01U) == 0U) {
            break;
        }
    }

    return bme280_read_calibration(port, address, i2c_mutex, cal);
}

esp_err_t bme280_read_calibration(i2c_port_t port, uint8_t address, SemaphoreHandle_t i2c_mutex, Bme280Calib *cal)
{
    if ((cal == nullptr) || (i2c_mutex == nullptr)) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t calib_1[26] = {}; // 0x88..0xA1
    uint8_t calib_2[7] = {};  // 0xE1..0xE7

    esp_err_t ret = bme280_read_registers(port, address, i2c_mutex, 0x88, calib_1, sizeof(calib_1));
    if (ret != ESP_OK) {
        return ret;
    }

    ret = bme280_read_registers(port, address, i2c_mutex, 0xE1, calib_2, sizeof(calib_2));
    if (ret != ESP_OK) {
        return ret;
    }

    cal->dig_T1 = u16_le(&calib_1[0]);
    cal->dig_T2 = s16_le(&calib_1[2]);
    cal->dig_T3 = s16_le(&calib_1[4]);

    cal->dig_P1 = u16_le(&calib_1[6]);
    cal->dig_P2 = s16_le(&calib_1[8]);
    cal->dig_P3 = s16_le(&calib_1[10]);
    cal->dig_P4 = s16_le(&calib_1[12]);
    cal->dig_P5 = s16_le(&calib_1[14]);
    cal->dig_P6 = s16_le(&calib_1[16]);
    cal->dig_P7 = s16_le(&calib_1[18]);
    cal->dig_P8 = s16_le(&calib_1[20]);
    cal->dig_P9 = s16_le(&calib_1[22]);

    cal->dig_H1 = calib_1[25];
    cal->dig_H2 = s16_le(&calib_2[0]);
    cal->dig_H3 = calib_2[2];
    cal->dig_H4 = (int16_t)(((int16_t)calib_2[3] << 4) | (calib_2[4] & 0x0F));
    cal->dig_H5 = (int16_t)(((int16_t)calib_2[5] << 4) | ((calib_2[4] >> 4) & 0x0F));
    cal->dig_H6 = (int8_t)calib_2[6];

    return ESP_OK;
}

Bme280Compensated bme280_compensate(int32_t raw_temp,
                                    int32_t raw_press,
                                    int32_t raw_hum,
                                    const Bme280Calib *cal)
{
    Bme280Compensated out = {};

    int32_t t_fine = bme280_compensate_temperature(raw_temp, cal);
    out.temperature_c = bme280_temperature_c_from_tfine(t_fine);
    out.pressure_hpa = bme280_compensate_pressure(raw_press, t_fine, cal);
    out.humidity_rh = bme280_compensate_humidity(raw_hum, t_fine, cal);

    return out;
}

esp_err_t bme280_read_measurement(i2c_port_t port,
                                  uint8_t address,
                                  SemaphoreHandle_t i2c_mutex,
                                  const Bme280Calib *cal,
                                  int16_t *temp_x100,
                                  uint16_t *hum_x100,
                                  uint16_t *pressure_mmhg)
{
    if ((cal == nullptr) ||
        (temp_x100 == nullptr) ||
        (hum_x100 == nullptr) ||
        (pressure_mmhg == nullptr) ||
        (i2c_mutex == nullptr)) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = bme280_write_register(port, address, i2c_mutex, BME280_CTRL_HUM_REG, 0x01);
    if (err != ESP_OK) {
        return err;
    }

    err = bme280_write_register(port, address, i2c_mutex, BME280_CTRL_MEAS_REG, 0x25);
    if (err != ESP_OK) {
        return err;
    }

    // Wait for the forced-mode measurement to complete.
    // The chip sets measuring=1 briefly after ctrl_meas is written, so we must
    // delay before polling to avoid a race where status reads 0 before the bit
    // is even set. Max measurement time for x1/x1/x1 oversampling is ~9.3 ms
    // per datasheet, so 10 ms covers the full window.
    vTaskDelay(pdMS_TO_TICKS(10));

    for (int attempt = 0; attempt < 5; ++attempt) {
        uint8_t status = 0;
        err = bme280_read_registers(port, address, i2c_mutex, BME280_STATUS_REG, &status, 1);
        if (err != ESP_OK) {
            return err;
        }
        if ((status & 0x08U) == 0U) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(3));
    }

    uint8_t raw[8] = {};
    err = bme280_read_registers(port, address, i2c_mutex, BME280_PRESS_MSB_REG, raw, sizeof(raw));
    if (err != ESP_OK) {
        return err;
    }

    const int32_t raw_press = ((int32_t)raw[0] << 12) |
                              ((int32_t)raw[1] << 4) |
                              ((int32_t)raw[2] >> 4);
    const int32_t raw_temp = ((int32_t)raw[3] << 12) |
                             ((int32_t)raw[4] << 4) |
                             ((int32_t)raw[5] >> 4);
    const int32_t raw_hum = ((int32_t)raw[6] << 8) | (int32_t)raw[7];

    const Bme280Compensated compensated = bme280_compensate(raw_temp, raw_press, raw_hum, cal);

    *temp_x100 = static_cast<int16_t>(lroundf(compensated.temperature_c * 100.0f));
    *hum_x100 = static_cast<uint16_t>(lroundf(compensated.humidity_rh * 100.0f));
    *pressure_mmhg = pressure_hpa_to_mmhg(compensated.pressure_hpa);

    return ESP_OK;
}


