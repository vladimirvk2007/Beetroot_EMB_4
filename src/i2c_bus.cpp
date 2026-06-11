#include "i2c_bus.h"

#include "esp_log.h"

esp_err_t i2c_bus_init(i2c_port_t port, gpio_num_t sda_pin, gpio_num_t scl_pin, uint32_t freq_hz) {
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = sda_pin;
    conf.scl_io_num = scl_pin;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = freq_hz;

    esp_err_t err = i2c_param_config(port, &conf);
    if (err != ESP_OK) {
        return err;
    }

    return i2c_driver_install(port, conf.mode, 0, 0, 0);
}

esp_err_t i2c_bus_ping_address(i2c_port_t port, uint8_t addr, TickType_t timeout) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, static_cast<uint8_t>((addr << 1) | I2C_MASTER_WRITE), true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(port, cmd, timeout);
    i2c_cmd_link_delete(cmd);
    return err;
}

void i2c_bus_scan(i2c_port_t port, TickType_t timeout, const char* log_tag) {
    const char* tag = (log_tag != nullptr) ? log_tag : "I2C";
    ESP_LOGI(tag, "I2C scan started");
    for (uint8_t addr = 1; addr < 0x7F; ++addr) {
        if (i2c_bus_ping_address(port, addr, timeout) == ESP_OK) {
            ESP_LOGI(tag, "I2C device found at 0x%02X", addr);
        }
    }
}
