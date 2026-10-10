#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
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

    // Малюємо просту фігуру на OLED 128x64.
    ESP_ERROR_CHECK(oled_clear(oled));
    ESP_ERROR_CHECK(oled_print(oled, 0, 1, false, "OLED drawing"));
    ESP_ERROR_CHECK(oled_draw_line(oled, 64, 14, 40, 34, false));
    ESP_ERROR_CHECK(oled_draw_line(oled, 64, 14, 88, 34, false));
    ESP_ERROR_CHECK(oled_draw_rectangle(oled, 45, 33, 38, 27, false));
    ESP_ERROR_CHECK(oled_fill_rectangle(oled, 59, 44, 10, 15, false));
    ESP_ERROR_CHECK(oled_draw_circle(oled, 64, 27, 4, false));

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
