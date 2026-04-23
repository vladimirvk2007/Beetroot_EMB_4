#include "bme280_spi.h"
#include "spi.h"

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

esp_err_t bme280_read_calibration(spi_device_handle_t spi, Bme280Calib *cal)
{
    if (cal == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t calib_1[26] = {}; // 0x88..0xA1
    uint8_t calib_2[7] = {};  // 0xE1..0xE7

    esp_err_t ret = spi_read_registers(spi, 0x88, calib_1, sizeof(calib_1));
    if (ret != ESP_OK) {
        return ret;
    }

    ret = spi_read_registers(spi, 0xE1, calib_2, sizeof(calib_2));
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


