#include "display.h"

#include <stdio.h>

#include "esp_check.h"

static const char* TAG = "DISPLAY";

static constexpr uint8_t SSD1306_WIDTH = 128;
static constexpr uint8_t SSD1306_HEIGHT = 64;
static constexpr uint16_t SSD1306_BUF_SIZE = SSD1306_WIDTH * SSD1306_HEIGHT / 8;

static uint8_t gDisplayBuf[SSD1306_BUF_SIZE] = {0};

static const uint8_t FONT5X7[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x5F, 0x00, 0x00},
    {0x00, 0x07, 0x00, 0x07, 0x00},
    {0x14, 0x7F, 0x14, 0x7F, 0x14},
    {0x24, 0x2A, 0x7F, 0x2A, 0x12},
    {0x23, 0x13, 0x08, 0x64, 0x62},
    {0x36, 0x49, 0x55, 0x22, 0x50},
    {0x00, 0x05, 0x03, 0x00, 0x00},
    {0x00, 0x1C, 0x22, 0x41, 0x00},
    {0x00, 0x41, 0x22, 0x1C, 0x00},
    {0x14, 0x08, 0x3E, 0x08, 0x14},
    {0x08, 0x08, 0x3E, 0x08, 0x08},
    {0x00, 0x50, 0x30, 0x00, 0x00},
    {0x08, 0x08, 0x08, 0x08, 0x08},
    {0x00, 0x60, 0x60, 0x00, 0x00},
    {0x20, 0x10, 0x08, 0x04, 0x02},
    {0x3E, 0x51, 0x49, 0x45, 0x3E},
    {0x00, 0x42, 0x7F, 0x40, 0x00},
    {0x42, 0x61, 0x51, 0x49, 0x46},
    {0x21, 0x41, 0x45, 0x4B, 0x31},
    {0x18, 0x14, 0x12, 0x7F, 0x10},
    {0x27, 0x45, 0x45, 0x45, 0x39},
    {0x3C, 0x4A, 0x49, 0x49, 0x30},
    {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36},
    {0x06, 0x49, 0x49, 0x29, 0x1E},
    {0x00, 0x36, 0x36, 0x00, 0x00},
    {0x00, 0x56, 0x36, 0x00, 0x00},
    {0x08, 0x14, 0x22, 0x41, 0x00},
    {0x14, 0x14, 0x14, 0x14, 0x14},
    {0x00, 0x41, 0x22, 0x14, 0x08},
    {0x02, 0x01, 0x51, 0x09, 0x06},
    {0x32, 0x49, 0x79, 0x41, 0x3E},
    {0x7E, 0x11, 0x11, 0x11, 0x7E},
    {0x7F, 0x49, 0x49, 0x49, 0x36},
    {0x3E, 0x41, 0x41, 0x41, 0x22},
    {0x7F, 0x41, 0x41, 0x22, 0x1C},
    {0x7F, 0x49, 0x49, 0x49, 0x41},
    {0x7F, 0x09, 0x09, 0x09, 0x01},
    {0x3E, 0x41, 0x49, 0x49, 0x7A},
    {0x7F, 0x08, 0x08, 0x08, 0x7F},
    {0x00, 0x41, 0x7F, 0x41, 0x00},
    {0x20, 0x40, 0x41, 0x3F, 0x01},
    {0x7F, 0x08, 0x14, 0x22, 0x41},
    {0x7F, 0x40, 0x40, 0x40, 0x40},
    {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    {0x7F, 0x04, 0x08, 0x10, 0x7F},
    {0x3E, 0x41, 0x41, 0x41, 0x3E},
    {0x7F, 0x09, 0x09, 0x09, 0x06},
    {0x3E, 0x41, 0x51, 0x21, 0x5E},
    {0x7F, 0x09, 0x19, 0x29, 0x46},
    {0x46, 0x49, 0x49, 0x49, 0x31},
    {0x01, 0x01, 0x7F, 0x01, 0x01},
    {0x3F, 0x40, 0x40, 0x40, 0x3F},
    {0x1F, 0x20, 0x40, 0x20, 0x1F},
    {0x3F, 0x40, 0x38, 0x40, 0x3F},
    {0x63, 0x14, 0x08, 0x14, 0x63},
    {0x07, 0x08, 0x70, 0x08, 0x07},
    {0x61, 0x51, 0x49, 0x45, 0x43},
    {0x00, 0x7F, 0x41, 0x41, 0x00},
    {0x02, 0x04, 0x08, 0x10, 0x20},
    {0x00, 0x41, 0x41, 0x7F, 0x00},
    {0x04, 0x02, 0x01, 0x02, 0x04},
    {0x40, 0x40, 0x40, 0x40, 0x40},
    {0x00, 0x01, 0x02, 0x04, 0x00},
    {0x20, 0x54, 0x54, 0x54, 0x78},
    {0x7F, 0x48, 0x44, 0x44, 0x38},
    {0x38, 0x44, 0x44, 0x44, 0x20},
    {0x38, 0x44, 0x44, 0x48, 0x7F},
    {0x38, 0x54, 0x54, 0x54, 0x18},
    {0x08, 0x7E, 0x09, 0x01, 0x02},
    {0x0C, 0x52, 0x52, 0x52, 0x3E},
    {0x7F, 0x08, 0x04, 0x04, 0x78},
    {0x00, 0x44, 0x7D, 0x40, 0x00},
    {0x20, 0x40, 0x44, 0x3D, 0x00},
    {0x7F, 0x10, 0x28, 0x44, 0x00},
    {0x00, 0x41, 0x7F, 0x40, 0x00},
    {0x7C, 0x04, 0x18, 0x04, 0x78},
    {0x7C, 0x08, 0x04, 0x04, 0x78},
    {0x38, 0x44, 0x44, 0x44, 0x38},
    {0x7C, 0x14, 0x14, 0x14, 0x08},
    {0x08, 0x14, 0x14, 0x18, 0x7C},
    {0x7C, 0x08, 0x04, 0x04, 0x08},
    {0x48, 0x54, 0x54, 0x54, 0x20},
    {0x04, 0x3F, 0x44, 0x40, 0x20},
    {0x3C, 0x40, 0x40, 0x20, 0x7C},
    {0x1C, 0x20, 0x40, 0x20, 0x1C},
    {0x3C, 0x40, 0x30, 0x40, 0x3C},
    {0x44, 0x28, 0x10, 0x28, 0x44},
    {0x0C, 0x50, 0x50, 0x50, 0x3C},
    {0x44, 0x64, 0x54, 0x4C, 0x44},
    {0x00, 0x08, 0x36, 0x41, 0x00},
    {0x00, 0x00, 0x7F, 0x00, 0x00},
    {0x00, 0x41, 0x36, 0x08, 0x00},
    {0x10, 0x08, 0x08, 0x10, 0x08}
};

static esp_err_t ssd1306_send_command(i2c_port_t port, uint8_t address, uint8_t cmd) {
    uint8_t payload[2] = {0x00, cmd};
    return i2c_master_write_to_device(port, address, payload, sizeof(payload), pdMS_TO_TICKS(100));
}

static esp_err_t ssd1306_send_data(i2c_port_t port, uint8_t address, const uint8_t* data, size_t size) {
    if ((data == nullptr) || (size == 0)) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t chunk[17];
    chunk[0] = 0x40;
    size_t offset = 0;
    while (offset < size) {
        size_t part = size - offset;
        if (part > 16) {
            part = 16;
        }
        for (size_t i = 0; i < part; ++i) {
            chunk[1 + i] = data[offset + i];
        }
        esp_err_t err = i2c_master_write_to_device(port, address, chunk, part + 1, pdMS_TO_TICKS(100));
        if (err != ESP_OK) {
            return err;
        }
        offset += part;
    }

    return ESP_OK;
}

static void ssd1306_clear_buffer() {
    for (uint16_t i = 0; i < SSD1306_BUF_SIZE; ++i) {
        gDisplayBuf[i] = 0x00;
    }
}

static void ssd1306_draw_char(uint8_t x, uint8_t page, char c) {
    if (page >= (SSD1306_HEIGHT / 8) || x > (SSD1306_WIDTH - 6)) {
        return;
    }

    uint8_t idx = 0;
    if ((c >= 32) && (c <= 126)) {
        idx = static_cast<uint8_t>(c - 32);
    }

    const uint16_t base = static_cast<uint16_t>(page) * SSD1306_WIDTH + x;
    for (uint8_t col = 0; col < 5; ++col) {
        gDisplayBuf[base + col] = FONT5X7[idx][col];
    }
    gDisplayBuf[base + 5] = 0x00;
}

static void ssd1306_draw_text(uint8_t x, uint8_t page, const char* text) {
    if (text == nullptr) {
        return;
    }

    uint8_t cursor = x;
    for (size_t i = 0; text[i] != '\0'; ++i) {
        if (cursor > (SSD1306_WIDTH - 6)) {
            break;
        }
        ssd1306_draw_char(cursor, page, text[i]);
        cursor = static_cast<uint8_t>(cursor + 6);
    }
}

static esp_err_t ssd1306_flush_buffer(i2c_port_t port, uint8_t address) {
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0x21), TAG, "SSD1306 set col mode failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0x00), TAG, "SSD1306 col start failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, SSD1306_WIDTH - 1), TAG, "SSD1306 col end failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0x22), TAG, "SSD1306 set page mode failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0x00), TAG, "SSD1306 page start failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, (SSD1306_HEIGHT / 8) - 1), TAG, "SSD1306 page end failed");
    return ssd1306_send_data(port, address, gDisplayBuf, SSD1306_BUF_SIZE);
}

esp_err_t display_init(i2c_port_t port, uint8_t address) {
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0xAE), TAG, "SSD1306 display off failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0x20), TAG, "SSD1306 mem mode cmd failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0x00), TAG, "SSD1306 horizontal mode failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0xB0), TAG, "SSD1306 page start failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0xC8), TAG, "SSD1306 COM scan failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0x00), TAG, "SSD1306 low col failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0x10), TAG, "SSD1306 high col failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0x40), TAG, "SSD1306 start line failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0x81), TAG, "SSD1306 contrast cmd failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0x7F), TAG, "SSD1306 contrast val failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0xA1), TAG, "SSD1306 seg remap failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0xA6), TAG, "SSD1306 normal display failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0xA8), TAG, "SSD1306 mux cmd failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0x3F), TAG, "SSD1306 mux val failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0xA4), TAG, "SSD1306 RAM content failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0xD3), TAG, "SSD1306 offset cmd failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0x00), TAG, "SSD1306 offset val failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0xD5), TAG, "SSD1306 clock cmd failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0x80), TAG, "SSD1306 clock val failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0xD9), TAG, "SSD1306 precharge cmd failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0xF1), TAG, "SSD1306 precharge val failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0xDA), TAG, "SSD1306 compins cmd failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0x12), TAG, "SSD1306 compins val failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0xDB), TAG, "SSD1306 vcomh cmd failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0x40), TAG, "SSD1306 vcomh val failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0x8D), TAG, "SSD1306 charge pump cmd failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0x14), TAG, "SSD1306 charge pump val failed");
    ESP_RETURN_ON_ERROR(ssd1306_send_command(port, address, 0xAF), TAG, "SSD1306 display on failed");

    ssd1306_clear_buffer();
    ESP_RETURN_ON_ERROR(ssd1306_flush_buffer(port, address), TAG, "SSD1306 first flush failed");
    return ESP_OK;
}

esp_err_t display_render_sample(i2c_port_t port, uint8_t address, const SensorData* sample) {
    if (sample == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    char line0[22];
    char line1[22];
    char line2[22];
    char line3[22];

    snprintf(line0,
             sizeof(line0),
             "%04u-%02u-%02u",
             static_cast<unsigned>(sample->rtc.year),
             static_cast<unsigned>(sample->rtc.month),
             static_cast<unsigned>(sample->rtc.day));
    snprintf(line1,
             sizeof(line1),
             "%02u:%02u:%02u",
             static_cast<unsigned>(sample->rtc.hour),
             static_cast<unsigned>(sample->rtc.minute),
             static_cast<unsigned>(sample->rtc.second));
    snprintf(line2,
             sizeof(line2),
             "T:%d.%02dC H:%u.%02u",
             static_cast<int>(sample->temperature_c_x100 / 100),
             static_cast<int>(sample->temperature_c_x100 % 100),
             static_cast<unsigned>(sample->humidity_pct_x100 / 100),
             static_cast<unsigned>(sample->humidity_pct_x100 % 100));
    snprintf(line3, sizeof(line3), "P:%u mmHg", static_cast<unsigned>(sample->pressure_mmhg));

    ssd1306_clear_buffer();
    ssd1306_draw_text(0, 0, line0);
    ssd1306_draw_text(0, 1, line1);
    ssd1306_draw_text(0, 3, line2);
    ssd1306_draw_text(0, 5, line3);
    return ssd1306_flush_buffer(port, address);
}
