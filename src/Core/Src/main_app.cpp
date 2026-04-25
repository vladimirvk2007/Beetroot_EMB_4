#include <string.h>
#include <stdio.h>
#include "main.h"
#include "usbd_cdc_if.h"
#include "led.h"

extern DMA_HandleTypeDef hdma_memtomem_dma2_stream0;

// Структура для безпечної фіксації результату
typedef struct {
    volatile uint8_t is_done;
    volatile uint32_t finish_ticks;
} dma_result_t;

dma_result_t dma_res = {0};

static void cdc_send_text(const char *text) {
    if (text == NULL) {
        return;
    }

    CDC_Transmit_FS((uint8_t *)text, strlen(text));
}

// Функція ініціалізації лічильника тактів DWT
void DWT_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

// Callback завершення DMA
void HAL_DMA_XferCpltCallback(DMA_HandleTypeDef *hdma) {
    dma_res.finish_ticks = DWT->CYCCNT; // 1. Фіксуємо такт завершення
    dma_res.is_done = 1;                // 2. Піднімаємо прапорець
}

void run_benchmark(void) {
    DWT_Init();
    const size_t DATA_SIZE = 1024; // 1024 слова (4 КБ)
    uint32_t src[DATA_SIZE], dst[DATA_SIZE];
    char line[128];

    // Початкове заповнення
    for(int i=0; i < DATA_SIZE; i++) src[i] = i;

    // --- ТЕСТ 1: memcpy (CPU повністю зайнятий) ---
    uint32_t t1 = DWT->CYCCNT;
    memcpy(dst, src, sizeof(src));
    uint32_t t2 = DWT->CYCCNT;
    uint32_t memcpy_ticks = t2 - t1;

    // --- ТЕСТ 2: DMA (CPU вільний майже одразу) ---
    dma_res.is_done = 0;
    uint32_t dma_start_call = DWT->CYCCNT;

    // Запускаємо асинхронне копіювання
    HAL_DMA_Start_IT(&hdma_memtomem_dma2_stream0, (uint32_t)src, (uint32_t)dst, DATA_SIZE);

    uint32_t dma_call_blocked_ticks = DWT->CYCCNT - dma_start_call;

    // CPU може виконувати інший код тут!
    while (!dma_res.is_done) {
        __NOP(); // Імітація корисної роботи або просто очікування
    }

    uint32_t total_hardware_ticks = dma_res.finish_ticks - dma_start_call;

    // --- ВИСНОВКИ В КОНСОЛЬ ---
    cdc_send_text("\r\n--- STM32 DMA M2M BENCHMARK (in CPU Ticks) ---\r\n");

    snprintf(line, sizeof(line),
             "1. memcpy blocked CPU for:  %lu ticks\r\n", memcpy_ticks);
    cdc_send_text(line);

    snprintf(line, sizeof(line),
             "2. DMA call blocked CPU for: %lu ticks\r\n", dma_call_blocked_ticks);
    cdc_send_text(line);

    cdc_send_text("----------------------------------------------\r\n");

    snprintf(line, sizeof(line), "CPU was free %lu times faster!\r\n",
             memcpy_ticks / dma_call_blocked_ticks);
    cdc_send_text(line);

    snprintf(line, sizeof(line),
             "Actual hardware copy time:   %lu ticks\r\n", total_hardware_ticks);
    cdc_send_text(line);
}

extern "C" void main_cpp() {
    const char *msg = "LED blinked!\r\n";
    Led led13(GPIOC, GPIO_PIN_13);

    run_benchmark();

    while(1) {
        led13.toggle();
        cdc_send_text(msg);
        //CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
        HAL_Delay(2000);
    }
}
