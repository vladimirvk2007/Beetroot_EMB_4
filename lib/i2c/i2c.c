#include "i2c.h"

#include <limits.h>
#include <string.h>

#include "esp_log.h"

#define I2C_GLITCH_IGNORE_CNT 7
#define I2C_DEFAULT_SPEED_HZ 100000U
#define I2C_MAX_REGISTER_WRITE 32U

static const char *TAG = "i2c";

static int timeout_to_ms(uint32_t timeout_ms)
{
    return (timeout_ms > INT_MAX) ? -1 : (int)timeout_ms;
}

esp_err_t i2c_bus_init(const i2c_bus_settings_t *settings, i2c_master_bus_handle_t *bus)
{
    if (settings == NULL || bus == NULL) {
        ESP_LOGE(TAG, "Invalid bus init arguments");
        return ESP_ERR_INVALID_ARG;
    }

    const i2c_master_bus_config_t bus_config = {
        .i2c_port = settings->port,
        .sda_io_num = settings->sda_pin,
        .scl_io_num = settings->scl_pin,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = I2C_GLITCH_IGNORE_CNT,
        .flags.enable_internal_pullup = settings->enable_internal_pullup,
    };

    const esp_err_t err = i2c_new_master_bus(&bus_config, bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus init failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "I2C port %d ready: SDA GPIO%d, SCL GPIO%d",
             (int)settings->port, (int)settings->sda_pin, (int)settings->scl_pin);
    return ESP_OK;
}

esp_err_t i2c_bus_deinit(i2c_master_bus_handle_t bus)
{
    if (bus == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_err_t err = i2c_del_master_bus(bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus deinit failed: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t i2c_bus_probe(i2c_master_bus_handle_t bus, uint16_t address, uint32_t timeout_ms)
{
    if (bus == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_master_probe(bus, address, timeout_to_ms(timeout_ms));
}

esp_err_t i2c_device_add(i2c_master_bus_handle_t bus,
                         const i2c_device_settings_t *settings,
                         i2c_master_dev_handle_t *device)
{
    if (bus == NULL || settings == NULL || device == NULL) {
        ESP_LOGE(TAG, "Invalid device add arguments");
        return ESP_ERR_INVALID_ARG;
    }

    const i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = settings->address,
        .scl_speed_hz = (settings->scl_speed_hz != 0U) ? settings->scl_speed_hz
                                                       : I2C_DEFAULT_SPEED_HZ,
    };

    const esp_err_t err = i2c_master_bus_add_device(bus, &dev_config, device);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C device 0x%02X add failed: %s",
                 (unsigned int)settings->address, esp_err_to_name(err));
    }
    return err;
}

esp_err_t i2c_device_remove(i2c_master_dev_handle_t device)
{
    if (device == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_master_bus_rm_device(device);
}

esp_err_t i2c_write(i2c_master_dev_handle_t device,
                    const uint8_t *data,
                    size_t size,
                    uint32_t timeout_ms)
{
    if (device == NULL || data == NULL || size == 0U) {
        ESP_LOGE(TAG, "Invalid I2C write arguments");
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_master_transmit(device, data, size, timeout_to_ms(timeout_ms));
}

esp_err_t i2c_read(i2c_master_dev_handle_t device,
                   uint8_t *data,
                   size_t size,
                   uint32_t timeout_ms)
{
    if (device == NULL || data == NULL || size == 0U) {
        ESP_LOGE(TAG, "Invalid I2C read arguments");
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_master_receive(device, data, size, timeout_to_ms(timeout_ms));
}

esp_err_t i2c_write_read(i2c_master_dev_handle_t device,
                         const uint8_t *write_data,
                         size_t write_size,
                         uint8_t *read_data,
                         size_t read_size,
                         uint32_t timeout_ms)
{
    if (device == NULL || write_data == NULL || write_size == 0U ||
        read_data == NULL || read_size == 0U) {
        ESP_LOGE(TAG, "Invalid I2C write-read arguments");
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_master_transmit_receive(device, write_data, write_size,
                                       read_data, read_size,
                                       timeout_to_ms(timeout_ms));
}

esp_err_t i2c_write_register(i2c_master_dev_handle_t device,
                             uint8_t reg,
                             const uint8_t *data,
                             size_t size,
                             uint32_t timeout_ms)
{
    if (device == NULL || (data == NULL && size != 0U) || size > I2C_MAX_REGISTER_WRITE) {
        ESP_LOGE(TAG, "Invalid I2C register write arguments");
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t buffer[1U + I2C_MAX_REGISTER_WRITE];
    buffer[0] = reg;
    if (size > 0U) {
        memcpy(&buffer[1], data, size);
    }

    return i2c_master_transmit(device, buffer, size + 1U, timeout_to_ms(timeout_ms));
}

esp_err_t i2c_read_register(i2c_master_dev_handle_t device,
                            uint8_t reg,
                            uint8_t *data,
                            size_t size,
                            uint32_t timeout_ms)
{
    return i2c_write_read(device, &reg, 1U, data, size, timeout_ms);
}
