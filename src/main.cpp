#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "at24c32.h"
#include "i2c.h"

// Параметри шини I2C для плати ESP32-S3.
#define I2C_MASTER_SCL_IO GPIO_NUM_9
#define I2C_MASTER_SDA_IO GPIO_NUM_8
#define I2C_MASTER_NUM I2C_NUM_0

// Адреса EEPROM залежить від рівнів на виводах A2..A0.
#define EEPROM_A_PINS 0U
#define EEPROM_START_ADDRESS 0x0000U
#define EEPROM_PAGE_SIZE AT24C32_PAGE_SIZE_BYTES
#define EEPROM_PAGE_COUNT (AT24C32_SIZE_BYTES / EEPROM_PAGE_SIZE)
#define EEPROM_TEXT_PREFIX "Counter: "
#define LOOP_DELAY_MS 500U

static const char *TAG = "app";

extern "C" void app_main()
{
    // Ініціалізуємо шину I2C.
    i2c_master_bus_handle_t bus = NULL;
    i2c_bus_settings_t bus_cfg = {};
    bus_cfg.port = I2C_MASTER_NUM;
    bus_cfg.sda_pin = I2C_MASTER_SDA_IO;
    bus_cfg.scl_pin = I2C_MASTER_SCL_IO;
    bus_cfg.enable_internal_pullup = true;
    ESP_ERROR_CHECK(i2c_bus_init(&bus_cfg, &bus));

    // Перевіряємо адреси, які може використовувати AT24C32.
    for (uint8_t pins = 0U; pins < 8U; ++pins) {
        const uint16_t address = (uint16_t)(AT24C32_BASE_ADDRESS | pins);
        if (i2c_bus_probe(bus, address, 10U) == ESP_OK) {
            ESP_LOGI(TAG, "I2C device found at 0x%02X", (unsigned int)address);
        }
    }

    // Створюємо дескриптор EEPROM і перевіряємо її доступність.
    at24c32_handle_t eeprom = NULL;
    esp_err_t err = at24c32_init(bus, EEPROM_A_PINS, &eeprom);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "AT24C32 initialization failed: %s", esp_err_to_name(err));
        return;
    }
    err = at24c32_is_ready(eeprom);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "AT24C32 at address 0x%02X is not responding: %s",
                 (unsigned int)(AT24C32_BASE_ADDRESS | EEPROM_A_PINS),
                 esp_err_to_name(err));
        (void)at24c32_deinit(eeprom);
        return;
    }
    ESP_LOGI(TAG, "AT24C32 connected");

    uint32_t page_index = 0U;
    while (true) {
        // Послідовно обходимо сторінки пам'яті по колу.
        const uint32_t address = EEPROM_START_ADDRESS +
                                 (page_index % EEPROM_PAGE_COUNT) * EEPROM_PAGE_SIZE;
        ++page_index;

        // Читаємо повну сторінку; окремий буфер додає нуль-термінатор для рядка.
        uint8_t page_data[EEPROM_PAGE_SIZE] = {0};
        char page_text[EEPROM_PAGE_SIZE + 1U] = {0};
        err = at24c32_read_page(eeprom, address, page_data);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Read at 0x%04lX failed: %s",
                     (unsigned long)address, esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(LOOP_DELAY_MS));
            continue;
        }
        memcpy(page_text, page_data, sizeof(page_data));

        // Знаходимо попереднє значення лічильника або починаємо з нуля.
        uint32_t number = 0U;
        const char *text = strstr(page_text, EEPROM_TEXT_PREFIX);
        if (text != NULL) {
            number = (uint32_t)strtoul(text + strlen(EEPROM_TEXT_PREFIX), NULL, 10);
            ESP_LOGI(TAG, "Read at 0x%04lX: \"%s\"",
                     (unsigned long)address, page_text);
        } else {
            ESP_LOGI(TAG, "Page 0x%04lX has no counter; starting from 0",
                     (unsigned long)address);
        }

        // Готуємо новий рядок і записуємо його повною EEPROM-сторінкою.
        ++number;
        uint8_t output[EEPROM_PAGE_SIZE] = {0};
        const int length = snprintf((char *)output, sizeof(output),
                                    "Addr 0x%04lX " EEPROM_TEXT_PREFIX "%lu",
                                    (unsigned long)address, (unsigned long)number);
        if (length < 0 || (size_t)length >= sizeof(output)) {
            ESP_LOGE(TAG, "Counter text does not fit in EEPROM page");
            vTaskDelay(pdMS_TO_TICKS(LOOP_DELAY_MS));
            continue;
        }

        err = at24c32_write_page(eeprom, address, output);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Written: \"%s\"", (const char *)output);
        } else {
            ESP_LOGE(TAG, "Write failed: %s", esp_err_to_name(err));
        }

        // Робимо паузу перед переходом до наступної сторінки.
        vTaskDelay(pdMS_TO_TICKS(LOOP_DELAY_MS));
    }
}
