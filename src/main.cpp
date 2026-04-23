#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "spi.h"
#include "bme280_spi.h"
#include "led.h"

#define LED_OUT		GPIO_NUM_16

static const char *TAG = "BME280_SPI";

extern spi_device_handle_t spi;

extern "C" void app_main() {
    Led led(LED_OUT);

    esp_err_t ret;

    // Ініціалізація SPI та підключення до BME280
    ret = spi_initialize(&spi);
    ESP_ERROR_CHECK(ret);

    // 3. Тестове читання ID чипа (Регістр 0xD0)
    // BME280: регістр chip_id = 0xD0, очікуване значення 0x60
    uint8_t chip_id = 0;
    ret = spi_read_registers(spi, 0xD0, &chip_id, 1);

    if (ret == ESP_OK && chip_id == 0x60) {
        ESP_LOGI(TAG, "Chip ID: 0x%02X (should be 0x60)", chip_id);
    } else {
        ESP_LOGE(TAG, "Transmission error!");
    }

    Bme280Calib calib = {};
    ret = bme280_read_calibration(spi, &calib);
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Calib T: T1=%u T2=%d T3=%d", calib.dig_T1, calib.dig_T2, calib.dig_T3);
    ESP_LOGI(TAG, "Calib P: P1=%u P2=%d P3=%d P4=%d P5=%d P6=%d P7=%d P8=%d P9=%d",
             calib.dig_P1, calib.dig_P2, calib.dig_P3, calib.dig_P4, calib.dig_P5,
             calib.dig_P6, calib.dig_P7, calib.dig_P8, calib.dig_P9);
    ESP_LOGI(TAG, "Calib H: H1=%u H2=%d H3=%u H4=%d H5=%d H6=%d",
             calib.dig_H1, calib.dig_H2, calib.dig_H3, calib.dig_H4, calib.dig_H5, calib.dig_H6);

    while (1) {

        // регістр 0xF2 — це ctrl_hum:
        // [2:0] osrs_h = 001 — вологість x1 oversampling
        // Важливо: після запису в ctrl_hum потрібно записати ctrl_meas,
        // щоб нові налаштування вологості застосувалися.
        ret = spi_write_register(spi, 0xF2, 0x01);
        ESP_ERROR_CHECK(ret);

        // регістр 0xF4 — це ctrl_meas:
        // Біти	    Назва	Значення в 0x25
        // [7:5]	osrs_t	001 — температура x1 oversampling
        // [4:2]	osrs_p	001 — тиск x1 oversampling
        // [1:0]	mode	01 — forced mode (одне вимірювання)
        ret = spi_write_register(spi, 0xF4, 0x25);
        ESP_ERROR_CHECK(ret);

        vTaskDelay(10 / portTICK_PERIOD_MS);

        // Тиск 20-бітний, розподілений по регістрах:
        // 0xF7 — біти [19:12]
        // 0xF8 — біти [11:4]
        // 0xF9 — біти [3:0] у бітах [7:4]
        uint8_t press_raw[3] = {};
        ret = spi_read_registers(spi, 0xF7, press_raw, 3);
        ESP_ERROR_CHECK(ret);

        int32_t pressure = ((int32_t)press_raw[0] << 12) |
                           ((int32_t)press_raw[1] << 4)  |
                           ((int32_t)press_raw[2] >> 4);

        ESP_LOGI(TAG, "raw press: 0x%5X (%d)", pressure, pressure);

        // Температура 20-бітна, розподілена по регістрах:
        // 0xFA — біти [19:12] (8 біт)
        // 0xFB — біти [11:4] (8 біт)
        // 0xFC — біти [3:0] у бітах [7:4] регістра (4 біт, молодші 4 біти завжди 0000)
        uint8_t temp_raw[3] = {};
        ret = spi_read_registers(spi, 0xFA, temp_raw, 3);
        ESP_ERROR_CHECK(ret);

        int32_t temperature = ((int32_t)temp_raw[0] << 12) |
                        ((int32_t)temp_raw[1] << 4)  |
                        ((int32_t)temp_raw[2] >> 4);

        ESP_LOGI(TAG, "raw temp: 0x%5X (%d)", temperature, temperature);

        // Вологість 16-бітна, розподілена по регістрах:
        // 0xFD — старший байт
        // 0xFE — молодший байт
        uint8_t hum_raw[2] = {};
        ret = spi_read_registers(spi, 0xFD, hum_raw, 2);
        ESP_ERROR_CHECK(ret);

        int32_t humidity = ((int32_t)hum_raw[0] << 8) | (int32_t)hum_raw[1];
        ESP_LOGI(TAG, "raw hum:   0x%4X (%d)", humidity, humidity);

        Bme280Compensated value = bme280_compensate(temperature, pressure, humidity, &calib);
        ESP_LOGI(TAG, "comp temp: %.2f C, press: %.2f hPa, hum: %.2f %%RH",
             value.temperature_c, value.pressure_hpa, value.humidity_rh);

        // Блимання світлодіодом для візуального підтвердження роботи
        led.on();
        vTaskDelay(500 / portTICK_PERIOD_MS);
        led.off();
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

