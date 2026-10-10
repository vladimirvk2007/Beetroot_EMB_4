#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ds1307.h"
#include "i2c.h"
#include "oled.h"

static const char *TAG = "app";

// Піни та порт шини I2C
#define I2C_MASTER_SCL_IO  GPIO_NUM_9
#define I2C_MASTER_SDA_IO  GPIO_NUM_8
#define I2C_MASTER_NUM     I2C_NUM_0

extern "C" void app_main()
{
    // 1. Створюємо шину I2C
    i2c_master_bus_handle_t bus = NULL;
    i2c_bus_settings_t bus_cfg = {};
    bus_cfg.port = I2C_MASTER_NUM;
    bus_cfg.sda_pin = I2C_MASTER_SDA_IO;
    bus_cfg.scl_pin = I2C_MASTER_SCL_IO;
    bus_cfg.enable_internal_pullup = true;
    ESP_ERROR_CHECK(i2c_bus_init(&bus_cfg, &bus));

    // 2. Ініціалізуємо дисплей 128x64 (адреса 0x3C, 400 кГц за замовчуванням)
    oled_settings_t oled_cfg = OLED_SETTINGS_128X64_DEFAULT();
    oled_handle_t oled = NULL;
    esp_err_t err = oled_init(bus, &oled_cfg, &oled);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OLED init failed: %s", esp_err_to_name(err));
        return;
    }

    ds1307_handle_t rtc = NULL;
    err = ds1307_init(bus, &rtc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "DS1307 init failed: %s", esp_err_to_name(err));
        return;
    }

    // Встановлюємо дату й час із моменту запиту для перевірки RTC.
    const ds1307_datetime_t initial_datetime = {
        .year = 2026,
        .month = 10,
        .day = 10,
        .weekday = 7,
        .hour = 18,
        .minute = 48,
        .second = 20,
    };
    ESP_ERROR_CHECK(ds1307_set_datetime(rtc, &initial_datetime));

    ESP_ERROR_CHECK(oled_clear(oled));
    ESP_ERROR_CHECK(oled_print(oled, 0, 1, false, "DS1307 RTC"));

    while (1) {
        ds1307_datetime_t datetime;
        err = ds1307_get_datetime(rtc, &datetime);
        if (err == ESP_OK) {
            ESP_ERROR_CHECK(oled_printf(oled, 2, 1, "%04u-%02u-%02u",
                                        (unsigned int)datetime.year,
                                        (unsigned int)datetime.month,
                                        (unsigned int)datetime.day));
            ESP_ERROR_CHECK(oled_printf(oled, 4, 2, "%02u:%02u:%02u",
                                        (unsigned int)datetime.hour,
                                        (unsigned int)datetime.minute,
                                        (unsigned int)datetime.second));
        } else {
            ESP_LOGE(TAG, "DS1307 read failed: %s", esp_err_to_name(err));
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
