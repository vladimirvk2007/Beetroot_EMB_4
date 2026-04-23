#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"

#define LED_OUT		GPIO_NUM_16

// SPI pin definitions
#define PIN_NUM_MISO 13   // SDO
#define PIN_NUM_MOSI 11   // SDA
#define PIN_NUM_CLK  12   // SCK
#define PIN_NUM_CS   10   // CS

#define SPI_MAX_TRANSFER_SIZE 32

static const char *TAG = "BME280_SPI";

class Led {
public:
    Led(gpio_num_t pin) : pin_(pin) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << pin_),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_conf);
    }
    void on() const {
        gpio_set_level(pin_, 0);
    }
    void off() const {
        gpio_set_level(pin_, 1);
    }
private:
    const gpio_num_t pin_;
};

esp_err_t spi_write_register(spi_device_handle_t spi, uint8_t reg, uint8_t value)
{
    esp_err_t ret = ESP_OK;

    uint8_t tx_buf[2] = {
        (uint8_t)(reg & ~0x80), // біт запису: MSB = 0
        value
    };

    spi_transaction_t t = {};
    t.length    = 16;
    t.tx_buffer = tx_buf;

    ret = spi_device_transmit(spi, &t);

    return ret;
}

esp_err_t spi_read_registers(spi_device_handle_t spi, uint8_t reg,
                              uint8_t *rx_buf, size_t num_bytes)
{
    if (num_bytes == 0 || rx_buf == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    const size_t total = num_bytes + 1; // 1 байт адреси + num_bytes даних
    if (total > SPI_MAX_TRANSFER_SIZE) {
        return ESP_ERR_INVALID_SIZE;
    }

    uint8_t tx_buf[SPI_MAX_TRANSFER_SIZE] = {};
    uint8_t rx_tmp[SPI_MAX_TRANSFER_SIZE] = {};

    tx_buf[0] = reg | 0x80; // Біт читання для BME280

    spi_transaction_t t = {};
    t.length    = total * 8;
    t.tx_buffer = tx_buf;
    t.rx_buffer = rx_tmp;

    esp_err_t ret = spi_device_transmit(spi, &t);

    if (ret == ESP_OK) {
        memcpy(rx_buf, rx_tmp + 1, num_bytes); // пропускаємо перший байт (відповідь на адресу)

        char buf[num_bytes * 3 + 1];
        buf[0] = '\0';
        for (size_t i = 0; i < num_bytes; i++) {
            snprintf(buf + i * 3, 4, "%02X ", rx_buf[i]);
        }
        ESP_LOGI(TAG, "spi_read reg=0x%02X: %s", reg, buf);
    }

    return ret;
}

extern "C" void app_main() {
    Led led(LED_OUT);

    esp_err_t ret;
    spi_device_handle_t spi;

    // 1. Конфігурація шини SPI
    spi_bus_config_t buscfg = {};
    buscfg.miso_io_num = PIN_NUM_MISO;
    buscfg.mosi_io_num = PIN_NUM_MOSI;
    buscfg.sclk_io_num = PIN_NUM_CLK;
    buscfg.max_transfer_sz = SPI_MAX_TRANSFER_SIZE;
    buscfg.data_io_default_level = true;
    buscfg.flags = SPICOMMON_BUSFLAG_MASTER |
                    SPICOMMON_BUSFLAG_SCLK |
                    SPICOMMON_BUSFLAG_MOSI |
                    SPICOMMON_BUSFLAG_MISO;
    buscfg.isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;

     // Ініціалізація шини (використовуємо SPI2_HOST)
    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_DISABLED);
    ESP_ERROR_CHECK(ret);

    // 2. Конфігурація пристрою (Slave)
    spi_device_interface_config_t devcfg = {};
    devcfg.clock_speed_hz = 1 * 1000 * 1000;     // Швидкість 1 МГц
    devcfg.mode = 0;                             // SPI Mode 0 (CPOL=0, CPHA=0)
    devcfg.spics_io_num = PIN_NUM_CS;            // CS пін
    devcfg.queue_size = 7;                       // Черга транзакцій
    devcfg.clock_source = SPI_CLK_SRC_DEFAULT;   // Джерело тактового сигналу

    // Додаємо пристрій на шину
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);

    // 3. Тестове читання ID чипа (Регістр 0xD0)
    // BME280: регістр chip_id = 0xD0, очікуване значення 0x60
    uint8_t chip_id = 0;
    ret = spi_read_registers(spi, 0xD0, &chip_id, 1);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Chip ID: 0x%02X (should be 0x60)", chip_id);
    } else {
        ESP_LOGE(TAG, "Transmission error!");
    }

    while (1) {

        // регістр 0xF4 — це ctrl_meas:
        // Біти	    Назва	Значення в 0x21
        // [7:5]	osrs_t	001 — температура x1 oversampling
        // [4:2]	osrs_p	000 — тиск вимкнено
        // [1:0]	mode	01 — forced mode (одне вимірювання)

        // Запускаємо forced mode для вимірювання температури (без тиску)
        spi_write_register(spi, 0xF4, 0x21);
        vTaskDelay(10 / portTICK_PERIOD_MS);

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

        led.on();
        vTaskDelay(500 / portTICK_PERIOD_MS);
        led.off();
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

