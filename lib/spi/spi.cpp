#include <string.h>
#include "esp_log.h"
#include "spi.h"

// SPI pin definitions
#define PIN_NUM_MISO            13   // SDO
#define PIN_NUM_MOSI            11   // SDA
#define PIN_NUM_CLK             12   // SCK
#define PIN_NUM_CS              10   // CS
#define SPI_MAX_TRANSFER_SIZE   32

static const char *TAG = "SPI";

spi_device_handle_t spi;

esp_err_t spi_initialize(spi_device_handle_t *spi)
{
    esp_err_t ret = ESP_OK;

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
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, spi);
    ESP_ERROR_CHECK(ret);

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

