#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_async_memcpy.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "led.h"

#define LED_OUT		GPIO_NUM_16
#define DATA_SIZE	(128 * 1024) // 128 KB

static const char *TAG = "DMA_M2M_FLAG";

// Структура для передачі даних у Callback
typedef struct {
    volatile bool is_done;         // Атомарний прапорець (32 біт)
    volatile int64_t finish_time;  // Час завершення
} dma_result_t;

// Callback: виконується в контексті переривання (ISR)
static bool IRAM_ATTR dma_copy_done_cb(async_memcpy_t mcp_hdl,
                                        async_memcpy_event_t *event,
                                        void *cb_args) {
    dma_result_t *res = (dma_result_t *)cb_args;
    res->finish_time = esp_timer_get_time(); // 1. Записуємо час
    res->is_done = true;                     // 2. Піднімаємо прапорець (сигнал готовності)

    return false;
}

extern "C" void app_main() {
    Led led(LED_OUT);
    esp_err_t ret = ESP_OK;

    async_memcpy_config_t config = {};
    config.backlog = 8;
    config.dma_burst_size = 16;
    config.flags = 0;
    async_memcpy_handle_t driver = NULL;
    ret = esp_async_memcpy_install(&config, &driver);
    ESP_ERROR_CHECK(ret);

    // Підготовка пам'яті
    uint8_t *src = (uint8_t *)heap_caps_malloc(DATA_SIZE, MALLOC_CAP_DMA);
    uint8_t *dst = (uint8_t *)heap_caps_malloc(DATA_SIZE, MALLOC_CAP_DMA);
    if (!src || !dst) {
        ESP_LOGE(TAG, "Failed to allocate DMA-capable memory");
        free(src);
        free(dst);
        esp_async_memcpy_uninstall(driver);
        return;
    }

    while (1) {
        dma_result_t dma_res = { .is_done = false, .finish_time = 0 };

        ESP_LOGI(TAG, "--- Starting Test ---");

        // --- ТЕСТ: memcpy ---
        int64_t t1 = esp_timer_get_time();
        memcpy(dst, src, DATA_SIZE);
        int64_t memcpy_cpu_time = esp_timer_get_time() - t1;

        // --- ТЕСТ: DMA ---
        int64_t dma_start_call = esp_timer_get_time();
        ret = esp_async_memcpy(driver, dst, src, DATA_SIZE, dma_copy_done_cb, &dma_res);
        ESP_ERROR_CHECK(ret);
        int64_t dma_cpu_block_time = esp_timer_get_time() - dma_start_call;

        // Чекаємо на прапорець завершення DMA
        while (!dma_res.is_done) {
            esp_rom_delay_us(10);
        }

        ESP_LOGI(TAG, "1. memcpy (CPU busy):     %lld us", memcpy_cpu_time);
        ESP_LOGI(TAG, "2. DMA call (CPU busy):   %lld us", dma_cpu_block_time);
        ESP_LOGI(TAG, "Hardware copy time: %lld us", dma_res.finish_time - dma_start_call);

        // Блимання світлодіодом для візуалізації циклу
        led.on();
        vTaskDelay(500 / portTICK_PERIOD_MS);
        led.off();
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    free(src);
    free(dst);
    esp_async_memcpy_uninstall(driver);
}

