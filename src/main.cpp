#include <stdint.h>
#include <stdio.h>

#include "driver/i2c.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "app_types.h"
#include "bme280_i2c.h"
#include "display.h"
#include "ds1307.h"
#include "flash_logger.h"
#include "i2c_bus.h"
#include "power_manager.h"
#include "uart_console.h"

static const char* TAG = "APP";

// I2C config (adjust pins for your hardware if needed).
static constexpr i2c_port_t I2C_PORT = I2C_NUM_0;
static constexpr gpio_num_t I2C_SDA_PIN = GPIO_NUM_8;
static constexpr gpio_num_t I2C_SCL_PIN = GPIO_NUM_9;
static constexpr uint32_t I2C_FREQ_HZ = 100000;

static constexpr uint8_t DS1307_ADDR = 0x68;
static constexpr uint8_t SSD1306_ADDR = 0x3C;
static constexpr uint8_t BME280_ADDR = 0x76;

static constexpr TickType_t SENSOR_REQ_TIMEOUT = pdMS_TO_TICKS(200);
static constexpr TickType_t SENSOR_DATA_TIMEOUT = pdMS_TO_TICKS(500);
static constexpr TickType_t DISPLAY_DONE_TIMEOUT = pdMS_TO_TICKS(400);
static constexpr TickType_t LOGGER_QUEUE_TIMEOUT = pdMS_TO_TICKS(50);
static constexpr TickType_t STATE_MUTEX_TIMEOUT = pdMS_TO_TICKS(20);
static constexpr uint64_t WAKEUP_PERIOD_US = 1000000ULL;

struct SensorReq {
    uint32_t seq;
};

struct DisplayData {
    SensorData sample;
    bool show_log_full;
};

static QueueHandle_t qSensorReq = nullptr;
static QueueHandle_t qSensorData = nullptr;
static QueueHandle_t qDisplayData = nullptr;
static QueueHandle_t qDisplayDone = nullptr;
static QueueHandle_t qLogData = nullptr;
static SemaphoreHandle_t i2cMutex = nullptr;
static bool gDisplayReady = false;
static bool gBmeReady = false;
static esp_err_t gLastRtcErr = ESP_OK;
static Bme280Calib gBmeCalib = {};
static SensorData gLastSample = {};
static bool gHasSample = false;
static SemaphoreHandle_t gStateMutex = nullptr;
static UartConsoleContext gUartCtx = {};

static void sensor_task(void* pvParameters) {
    (void)pvParameters;

    SensorReq req = {};
    while (true) {
        if (xQueueReceive(qSensorReq, &req, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        SensorData data = {};
        data.seq = req.seq;

        const esp_err_t rtc_err = ds1307_read_time(I2C_PORT, DS1307_ADDR, i2cMutex, &data.rtc);
        bool bme_ready = false;
        if (xSemaphoreTake(gStateMutex, STATE_MUTEX_TIMEOUT) == pdTRUE) {
            gLastRtcErr = rtc_err;
            bme_ready = gBmeReady;
            xSemaphoreGive(gStateMutex);
        }

        data.rtc_ok = (rtc_err == ESP_OK);
        data.bme_ok = bme_ready &&
                      (bme280_read_measurement(I2C_PORT,
                                               BME280_ADDR,
                                               i2cMutex,
                                               &gBmeCalib,
                                               &data.temperature_c_x100,
                                               &data.humidity_pct_x100,
                                               &data.pressure_mmhg) == ESP_OK);

        const int temp_c_x10 = (data.temperature_c_x100 >= 0)
                       ? ((data.temperature_c_x100 + 5) / 10)
                       : ((data.temperature_c_x100 - 5) / 10);
        const int temp_frac = (temp_c_x10 % 10 >= 0) ? (temp_c_x10 % 10) : -(temp_c_x10 % 10);
        const unsigned hum_pct_x10 = static_cast<unsigned>((data.humidity_pct_x100 + 5U) / 10U);

        ESP_LOGI(TAG,
             "Sensor: seq=%lu rtc=%d(%s) bme=%d t=%d.%d h=%u.%u p=%u",
                 static_cast<unsigned long>(data.seq),
                 data.rtc_ok ? 1 : 0,
                 esp_err_to_name(rtc_err),
                 data.bme_ok ? 1 : 0,
             temp_c_x10 / 10,
             temp_frac,
             hum_pct_x10 / 10,
             hum_pct_x10 % 10,
                 static_cast<unsigned>(data.pressure_mmhg));

        if (xQueueSend(qSensorData, &data, pdMS_TO_TICKS(50)) != pdTRUE) {
            ESP_LOGW(TAG, "qSensorData overflow, sample dropped");
        }
    }
}

static void display_task(void* pvParameters) {
    (void)pvParameters;

    DisplayData frame = {};
    while (true) {
        if (xQueueReceive(qDisplayData, &frame, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        bool done = true;

        if (!gDisplayReady) {
            (void)xQueueOverwrite(qDisplayDone, &done);
            continue;
        }

        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(200)) != pdTRUE) {
            ESP_LOGW(TAG, "Display mutex timeout");
            (void)xQueueOverwrite(qDisplayDone, &done);
            continue;
        }

        esp_err_t err = ESP_OK;
        if (frame.show_log_full) {
            err = display_render_log_full(I2C_PORT, SSD1306_ADDR);
        } else {
            err = display_render_sample(I2C_PORT, SSD1306_ADDR, &frame.sample);
        }
        xSemaphoreGive(i2cMutex);

        if (err != ESP_OK) {
            ESP_LOGW(TAG, "SSD1306 render failed: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "Display: frame rendered");
        }

        (void)xQueueOverwrite(qDisplayDone, &done);
    }
}

static void manager_task(void* pvParameters) {
    (void)pvParameters;

    uint32_t seq = 0;
    SensorReq req = {};
    SensorData sample = {};
    DisplayData frame = {};
    TickType_t lastWake = xTaskGetTickCount();

    while (true) {
        req.seq = seq++;

        if (xQueueSend(qSensorReq, &req, SENSOR_REQ_TIMEOUT) != pdTRUE) {
            ESP_LOGW(TAG, "qSensorReq send timeout");
        } else if (xQueueReceive(qSensorData, &sample, SENSOR_DATA_TIMEOUT) == pdTRUE) {
            frame.sample = sample;
            frame.show_log_full = flash_logger_is_full();
            if (xSemaphoreTake(gStateMutex, STATE_MUTEX_TIMEOUT) == pdTRUE) {
                gLastSample = sample;
                gHasSample = true;
                xSemaphoreGive(gStateMutex);
            }

            if (flash_logger_is_enabled()) {
                if (xQueueSend(qLogData, &sample, LOGGER_QUEUE_TIMEOUT) != pdTRUE) {
                    ESP_LOGW(TAG, "qLogData overflow, log sample dropped");
                }
            }

            ESP_LOGI(TAG,
                     "Manager: seq=%lu time=%04u-%02u-%02u %02u:%02u:%02u",
                     static_cast<unsigned long>(sample.seq),
                     static_cast<unsigned>(sample.rtc.year),
                     static_cast<unsigned>(sample.rtc.month),
                     static_cast<unsigned>(sample.rtc.day),
                     static_cast<unsigned>(sample.rtc.hour),
                     static_cast<unsigned>(sample.rtc.minute),
                     static_cast<unsigned>(sample.rtc.second));
            (void)xQueueOverwrite(qDisplayData, &frame);
            bool done = false;
            if (xQueueReceive(qDisplayDone, &done, DISPLAY_DONE_TIMEOUT) != pdTRUE) {
                ESP_LOGW(TAG, "Display render ack timeout");
            }
        } else {
            ESP_LOGW(TAG, "qSensorData receive timeout");
        }

        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(1000));
    }
}

static void logger_task(void* pvParameters) {
    (void)pvParameters;

    SensorData sample = {};
    while (true) {
        if (xQueueReceive(qLogData, &sample, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        if (!flash_logger_is_enabled()) {
            continue;
        }

        if (flash_logger_is_full()) {
            continue;
        }

        const esp_err_t log_err = flash_logger_append(&sample);
        if (log_err == ESP_ERR_NO_MEM) {
            ESP_LOGW(TAG, "LOGDATA partition is full, logging stopped");
        } else if (log_err != ESP_OK) {
            ESP_LOGW(TAG, "flash_logger_append failed: %s", esp_err_to_name(log_err));
        }
    }
}

extern "C" void app_main() {
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_WARN);
    ESP_LOGI(TAG, "App start: logger enabled, monitor should show INFO logs");
    configure_power_management();

    i2cMutex = xSemaphoreCreateMutex();
    gStateMutex = xSemaphoreCreateMutex();
    qSensorReq = xQueueCreate(4, sizeof(SensorReq));
    qSensorData = xQueueCreate(4, sizeof(SensorData));
    qDisplayData = xQueueCreate(1, sizeof(DisplayData));
    qDisplayDone = xQueueCreate(1, sizeof(bool));
    qLogData = xQueueCreate(8, sizeof(SensorData));

    if ((i2cMutex == nullptr) ||
        (gStateMutex == nullptr) ||
        (qSensorReq == nullptr) ||
        (qSensorData == nullptr) ||
        (qDisplayData == nullptr) ||
        (qDisplayDone == nullptr) ||
        (qLogData == nullptr)) {
        ESP_LOGE(TAG, "Failed to create RTOS objects");
        return;
    }

    const esp_err_t i2cErr = i2c_bus_init(I2C_PORT, I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ_HZ);
    if (i2cErr != ESP_OK) {
        ESP_LOGE(TAG, "i2c init failed: %s", esp_err_to_name(i2cErr));
        return;
    }

    if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(300)) == pdTRUE) {
        i2c_bus_scan(I2C_PORT, pdMS_TO_TICKS(100), TAG);
        esp_err_t rtcProbe = i2c_bus_ping_address(I2C_PORT, DS1307_ADDR, pdMS_TO_TICKS(100));
        esp_err_t oledProbe = i2c_bus_ping_address(I2C_PORT, SSD1306_ADDR, pdMS_TO_TICKS(100));
        esp_err_t bmeProbe = i2c_bus_ping_address(I2C_PORT, BME280_ADDR, pdMS_TO_TICKS(100));
        xSemaphoreGive(i2cMutex);
        ESP_LOGI(TAG,
                 "I2C probe: DS1307(0x%02X)=%s SSD1306(0x%02X)=%s BME280(0x%02X)=%s",
                 DS1307_ADDR,
                 esp_err_to_name(rtcProbe),
                 SSD1306_ADDR,
                 esp_err_to_name(oledProbe),
                 BME280_ADDR,
                 esp_err_to_name(bmeProbe));
    }

    const esp_err_t bmeErr = bme280_init(I2C_PORT, BME280_ADDR, i2cMutex, &gBmeCalib);
    if (bmeErr == ESP_OK) {
        if (xSemaphoreTake(gStateMutex, STATE_MUTEX_TIMEOUT) == pdTRUE) {
            gBmeReady = true;
            xSemaphoreGive(gStateMutex);
        }
    } else {
        ESP_LOGW(TAG, "BME280 init failed: %s", esp_err_to_name(bmeErr));
    }

    if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(300)) == pdTRUE) {
        esp_err_t displayErr = display_init(I2C_PORT, SSD1306_ADDR);
        xSemaphoreGive(i2cMutex);
        if (displayErr == ESP_OK) {
            if (xSemaphoreTake(gStateMutex, STATE_MUTEX_TIMEOUT) == pdTRUE) {
                gDisplayReady = true;
                xSemaphoreGive(gStateMutex);
            }
        } else {
            ESP_LOGW(TAG, "SSD1306 init failed: %s", esp_err_to_name(displayErr));
        }
    }

    const esp_err_t log_init_err = flash_logger_init();
    if (log_init_err != ESP_OK) {
        ESP_LOGW(TAG, "flash logger disabled: %s", esp_err_to_name(log_init_err));
    }

    BaseType_t rcSensor = xTaskCreate(sensor_task, "SensorTask", 4096, nullptr, 5, nullptr);
    BaseType_t rcDisplay = xTaskCreate(display_task, "DisplayTask", 4096, nullptr, 3, nullptr);
    BaseType_t rcManager = xTaskCreate(manager_task, "ManagerTask", 4096, nullptr, 6, nullptr);
    BaseType_t rcLogger = xTaskCreate(logger_task, "LoggerTask", 4096, nullptr, 4, nullptr);

    gUartCtx.i2c_port = I2C_PORT;
    gUartCtx.ds1307_addr = DS1307_ADDR;
    gUartCtx.i2c_mutex = i2cMutex;
    gUartCtx.last_sample = &gLastSample;
    gUartCtx.has_sample = &gHasSample;
    gUartCtx.display_ready = &gDisplayReady;
    gUartCtx.bme_ready = &gBmeReady;
    gUartCtx.last_rtc_err = &gLastRtcErr;
    gUartCtx.state_mutex = gStateMutex;

    BaseType_t rcUart = xTaskCreate(uart_console_task, "UartTask", 4096, &gUartCtx, 2, nullptr);

    if ((rcSensor != pdPASS) ||
        (rcDisplay != pdPASS) ||
        (rcManager != pdPASS) ||
        (rcLogger != pdPASS) ||
        (rcUart != pdPASS)) {
        ESP_LOGE(TAG,
                 "Task create failed: sensor=%ld display=%ld manager=%ld logger=%ld uart=%ld",
                 static_cast<long>(rcSensor),
                 static_cast<long>(rcDisplay),
                 static_cast<long>(rcManager),
                 static_cast<long>(rcLogger),
                 static_cast<long>(rcUart));
    }
}
