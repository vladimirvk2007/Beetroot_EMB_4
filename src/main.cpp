#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "i2c.h"

static const char *TAG = "app";

#define I2C_SDA_PIN GPIO_NUM_8
#define I2C_SCL_PIN GPIO_NUM_9
#define I2C_TIMEOUT_MS 100

#define DS1307_ADDRESS 0x68
#define DS1307_REG_START 0x00
#define DS1307_REG_COUNT 8

extern "C" void app_main()
{
    const i2c_bus_settings_t bus_settings = {
        .port = I2C_NUM_0,
        .sda_pin = I2C_SDA_PIN,
        .scl_pin = I2C_SCL_PIN,
        .enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus = NULL;
    esp_err_t err = i2c_bus_init(&bus_settings, &bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus initialization failed: %s", esp_err_to_name(err));
        return;
    }

    err = i2c_bus_probe(bus, DS1307_ADDRESS, I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "DS1307 not found: %s", esp_err_to_name(err));
        return;
    }

    const i2c_device_settings_t device_settings = {
        .address = DS1307_ADDRESS,
        .scl_speed_hz = 100000,
    };
    i2c_master_dev_handle_t rtc = NULL;
    err = i2c_device_add(bus, &device_settings, &rtc);
    if (err != ESP_OK) {
        return;
    }

    while (1) {
        uint8_t regs[DS1307_REG_COUNT];
        err = i2c_read_register(rtc, DS1307_REG_START, regs, sizeof(regs), I2C_TIMEOUT_MS);
        if (err == ESP_OK) {
            ESP_LOG_BUFFER_HEX(TAG, regs, sizeof(regs));
        } else {
            ESP_LOGE(TAG, "Register read failed: %s", esp_err_to_name(err));
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
