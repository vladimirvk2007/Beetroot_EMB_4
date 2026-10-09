#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "i2c.h"
#include "ssd1306.h"

static const char *TAG = "app";

#define I2C_MASTER_SCL_IO  GPIO_NUM_9
#define I2C_MASTER_SDA_IO  GPIO_NUM_8
#define I2C_MASTER_NUM     I2C_NUM_0

extern "C" void app_main()
{
    i2c_master_bus_handle_t bus = NULL;
    i2c_bus_settings_t bus_cfg = {};
    bus_cfg.port = I2C_MASTER_NUM;
    bus_cfg.sda_pin = I2C_MASTER_SDA_IO;
    bus_cfg.scl_pin = I2C_MASTER_SCL_IO;
    bus_cfg.enable_internal_pullup = true;
    ESP_ERROR_CHECK(i2c_bus_init(&bus_cfg, &bus));

    ssd1306_config_t oled_conf = I2C_SSD1306_128x64_CONFIG_DEFAULT;
    oled_conf.display_enabled = true;

    ssd1306_handle_t oled_dev = NULL;
    esp_err_t err = ssd1306_init(bus, &oled_conf, &oled_dev);
    if (err != ESP_OK || oled_dev == NULL) {
        ESP_LOGE(TAG, "SSD1306 init failed: %s", esp_err_to_name(err));
        return;
    }

    ssd1306_clear_display(oled_dev, false);
    ssd1306_display_text(oled_dev, 0, "Hello, ESP-IDF!", false);
    ssd1306_display_text(oled_dev, 2, "PlatformIO OK", false);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
