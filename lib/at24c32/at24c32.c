#include "at24c32.h"

#include <stdbool.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c.h"

static const char *TAG = "at24c32";

#define AT24C32_READY_PROBE_TIMEOUT_MS 10U

struct at24c32_s {
    i2c_master_bus_handle_t bus;
    i2c_master_dev_handle_t device;
    uint16_t address;
};

/* Перевіряє, чи діапазон повністю вміщується в EEPROM. */
static bool range_is_valid(uint32_t address, uint32_t size)
{
    return address <= AT24C32_SIZE_BYTES &&
           size <= AT24C32_SIZE_BYTES - address;
}

/* Очікує завершення внутрішнього циклу запису EEPROM. */
static esp_err_t wait_until_ready(at24c32_handle_t memory)
{
    const int64_t deadline_us = esp_timer_get_time() +
                                (int64_t)AT24C32_WRITE_CYCLE_MS * 1000;

    do {
        const esp_err_t err = i2c_bus_probe(memory->bus, memory->address,
                                            AT24C32_READY_PROBE_TIMEOUT_MS);
        if (err == ESP_OK) {
            return ESP_OK;
        }
        if (err != ESP_ERR_NOT_FOUND && err != ESP_ERR_TIMEOUT) {
            ESP_LOGE(TAG, "EEPROM ready check failed: %s", esp_err_to_name(err));
            return err;
        }
        vTaskDelay(1);
    } while (esp_timer_get_time() < deadline_us);

    ESP_LOGE(TAG, "EEPROM write cycle timed out");
    return ESP_ERR_TIMEOUT;
}

/* Додає EEPROM до шини та створює її дескриптор. */
esp_err_t at24c32_init(i2c_master_bus_handle_t bus,
                       uint8_t a_pins,
                       at24c32_handle_t *memory)
{
    if (bus == NULL || memory == NULL || a_pins > 7U) {
        return ESP_ERR_INVALID_ARG;
    }
    *memory = NULL;

    struct at24c32_s *ctx = calloc(1U, sizeof(*ctx));
    if (ctx == NULL) {
        return ESP_ERR_NO_MEM;
    }
    ctx->bus = bus;
    ctx->address = (uint16_t)(AT24C32_BASE_ADDRESS | a_pins);

    const i2c_device_settings_t settings = {
        .address = ctx->address,
        .scl_speed_hz = AT24C32_I2C_SPEED_HZ,
    };
    const esp_err_t err = i2c_device_add(bus, &settings, &ctx->device);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "AT24C32 initialization failed: %s", esp_err_to_name(err));
        free(ctx);
        return err;
    }

    *memory = ctx;
    return ESP_OK;
}

/* Видаляє пристрій із шини та звільняє дескриптор. */
esp_err_t at24c32_deinit(at24c32_handle_t memory)
{
    if (memory == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_err_t err = i2c_device_remove(memory->device);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "AT24C32 deinitialization failed: %s", esp_err_to_name(err));
        return err;
    }
    free(memory);
    return ESP_OK;
}

/* Перевіряє доступність EEPROM за її адресою I2C. */
esp_err_t at24c32_is_ready(at24c32_handle_t memory)
{
    if (memory == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_bus_probe(memory->bus, memory->address, AT24C32_I2C_TIMEOUT_MS);
}

/* Читає дані, попередньо передаючи 16-бітну адресу пам'яті. */
esp_err_t at24c32_read(at24c32_handle_t memory,
                       uint32_t address,
                       uint8_t *data,
                       uint32_t size)
{
    if (memory == NULL || !range_is_valid(address, size) ||
        (data == NULL && size != 0U)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (size == 0U) {
        return ESP_OK;
    }

    const uint8_t word_address[2] = {
        (uint8_t)(address >> 8U),
        (uint8_t)address,
    };
    const esp_err_t err = i2c_write_read(memory->device,
                                         word_address, sizeof(word_address),
                                         data, size, AT24C32_I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "EEPROM read failed at 0x%04lX: %s",
                 (unsigned long)address, esp_err_to_name(err));
    }
    return err;
}

/* Записує дані частинами, не перетинаючи межі сторінок. */
esp_err_t at24c32_write(at24c32_handle_t memory,
                        uint32_t address,
                        const uint8_t *data,
                        uint32_t size)
{
    if (memory == NULL || !range_is_valid(address, size) ||
        (data == NULL && size != 0U)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (size == 0U) {
        return ESP_OK;
    }

    uint8_t buffer[2U + AT24C32_PAGE_SIZE_BYTES];
    uint32_t written = 0U;
    while (written < size) {
        const uint32_t current_address = address + written;
        const uint32_t page_offset = current_address % AT24C32_PAGE_SIZE_BYTES;
        uint32_t chunk_size = AT24C32_PAGE_SIZE_BYTES - page_offset;
        if (chunk_size > size - written) {
            chunk_size = size - written;
        }

        buffer[0] = (uint8_t)(current_address >> 8U);
        buffer[1] = (uint8_t)current_address;
        for (uint32_t i = 0U; i < chunk_size; ++i) {
            buffer[2U + i] = data[written + i];
        }

        esp_err_t err = i2c_write(memory->device, buffer, 2U + chunk_size,
                                  AT24C32_I2C_TIMEOUT_MS);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "EEPROM write failed at 0x%04lX: %s",
                     (unsigned long)current_address, esp_err_to_name(err));
            return err;
        }

        err = wait_until_ready(memory);
        if (err != ESP_OK) {
            return err;
        }
        written += chunk_size;
    }
    return ESP_OK;
}

/* Перевіряє адресу початку сторінки та читає її повністю. */
esp_err_t at24c32_read_page(at24c32_handle_t memory,
                            uint32_t page_address,
                            uint8_t data[AT24C32_PAGE_SIZE_BYTES])
{
    if (memory == NULL || data == NULL ||
        (page_address % AT24C32_PAGE_SIZE_BYTES) != 0U ||
        !range_is_valid(page_address, AT24C32_PAGE_SIZE_BYTES)) {
        return ESP_ERR_INVALID_ARG;
    }

    return at24c32_read(memory, page_address, data, AT24C32_PAGE_SIZE_BYTES);
}

/* Записує повну сторінку однією транзакцією I2C. */
esp_err_t at24c32_write_page(at24c32_handle_t memory,
                             uint32_t page_address,
                             const uint8_t data[AT24C32_PAGE_SIZE_BYTES])
{
    if (memory == NULL || data == NULL ||
        (page_address % AT24C32_PAGE_SIZE_BYTES) != 0U ||
        !range_is_valid(page_address, AT24C32_PAGE_SIZE_BYTES)) {
        return ESP_ERR_INVALID_ARG;
    }

    return at24c32_write(memory, page_address, data, AT24C32_PAGE_SIZE_BYTES);
}

/* Читає один байт із заданої адреси. */
esp_err_t at24c32_read_byte(at24c32_handle_t memory,
                            uint32_t address,
                            uint8_t *value)
{
    if (value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return at24c32_read(memory, address, value, 1U);
}

/* Записує один байт за заданою адресою. */
esp_err_t at24c32_write_byte(at24c32_handle_t memory,
                             uint32_t address,
                             uint8_t value)
{
    return at24c32_write(memory, address, &value, 1U);
}

/* Заповнює вказаний діапазон, записуючи дані сторінками. */
esp_err_t at24c32_fill(at24c32_handle_t memory,
                       uint32_t address,
                       uint32_t size,
                       uint8_t value)
{
    if (memory == NULL || !range_is_valid(address, size)) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t buffer[AT24C32_PAGE_SIZE_BYTES];
    for (uint32_t i = 0U; i < sizeof(buffer); ++i) {
        buffer[i] = value;
    }

    uint32_t filled = 0U;
    while (filled < size) {
        uint32_t chunk_size = AT24C32_PAGE_SIZE_BYTES -
                              ((address + filled) % AT24C32_PAGE_SIZE_BYTES);
        if (chunk_size > size - filled) {
            chunk_size = size - filled;
        }
        const esp_err_t err = at24c32_write(memory, address + filled,
                                            buffer, chunk_size);
        if (err != ESP_OK) {
            return err;
        }
        filled += chunk_size;
    }
    return ESP_OK;
}
