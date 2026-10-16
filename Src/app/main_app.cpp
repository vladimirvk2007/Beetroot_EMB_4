#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "i2c/i2c.h"

static DMA_HandleTypeDef hdma_memtomem_dma2_stream0;

// Структура для безпечної фіксації результату
typedef struct {
    volatile uint8_t is_done;
    volatile uint32_t finish_ticks;
} dma_result_t;

dma_result_t dma_res = {0};

// Функція ініціалізації лічильника тактів DWT
void DWT_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

// Callback завершення DMA (реєструється через hdma->XferCpltCallback)
static void dma_xfer_cplt_callback(DMA_HandleTypeDef *hdma) {
    dma_res.finish_ticks = DWT->CYCCNT; // 1. Фіксуємо такт завершення
    dma_res.is_done = 1;                // 2. Піднімаємо прапорець
}

// Налаштування DMA2 Stream0 (memory-to-memory, 32 біти) і реєстрація callback
static void dma_init(void) {
    __HAL_RCC_DMA2_CLK_ENABLE();

    hdma_memtomem_dma2_stream0.Instance = DMA2_Stream0;
    hdma_memtomem_dma2_stream0.Init.Channel = DMA_CHANNEL_0;
    hdma_memtomem_dma2_stream0.Init.Direction = DMA_MEMORY_TO_MEMORY;
    hdma_memtomem_dma2_stream0.Init.PeriphInc = DMA_PINC_ENABLE;
    hdma_memtomem_dma2_stream0.Init.MemInc = DMA_MINC_ENABLE;
    hdma_memtomem_dma2_stream0.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_memtomem_dma2_stream0.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_memtomem_dma2_stream0.Init.Mode = DMA_NORMAL;
    hdma_memtomem_dma2_stream0.Init.Priority = DMA_PRIORITY_LOW;
    hdma_memtomem_dma2_stream0.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    hdma_memtomem_dma2_stream0.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    hdma_memtomem_dma2_stream0.Init.MemBurst = DMA_MBURST_SINGLE;
    hdma_memtomem_dma2_stream0.Init.PeriphBurst = DMA_PBURST_SINGLE;
    HAL_DMA_Init(&hdma_memtomem_dma2_stream0);

    hdma_memtomem_dma2_stream0.XferCpltCallback = dma_xfer_cplt_callback;

    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}

extern "C" void DMA2_Stream0_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_memtomem_dma2_stream0);
}

// Запуск DMA-копіювання і очікування завершення.
static uint32_t dma_memcpy(void *dst, const void *src, size_t words) {
    dma_res.is_done = 0;

    uint32_t t_start = DWT->CYCCNT;
    HAL_DMA_Start_IT(&hdma_memtomem_dma2_stream0,
                        (uint32_t)src,
                        (uint32_t)dst,
                        words);
    uint32_t blocked_ticks = DWT->CYCCNT - t_start;

    while (!dma_res.is_done) {
        __NOP();
    }

    return blocked_ticks;
}

void run_benchmark(void) {
    const size_t DATA_SIZE = 1024; // 1024 слова (4 КБ)
    uint32_t *src = (uint32_t *)malloc(DATA_SIZE * sizeof(uint32_t));
    uint32_t *dst = (uint32_t *)malloc(DATA_SIZE * sizeof(uint32_t));
    char line[128];

    if ((src == NULL) || (dst == NULL)) {
        printf("DMA benchmark: allocation failed\r\n");
        free(src);
        free(dst);
        return;
    }

    // --- ТЕСТ 1: memcpy (CPU повністю зайнятий) ---
    uint32_t t1 = DWT->CYCCNT;
    memcpy(dst, src, DATA_SIZE * sizeof(uint32_t));
    uint32_t memcpy_ticks = DWT->CYCCNT - t1;

    // --- ТЕСТ 2: DMA (CPU вільний майже одразу) ---
    uint32_t dma_start_call = DWT->CYCCNT;
    uint32_t dma_call_blocked_ticks = dma_memcpy(dst, src, DATA_SIZE);
    uint32_t total_hardware_ticks = dma_res.finish_ticks - dma_start_call;

    // --- ВИСНОВКИ ---
    printf("\r\n--- STM32 DMA M2M BENCHMARK (in CPU Ticks) ---\r\n");

    snprintf(line, sizeof(line),
             "1. memcpy blocked CPU for:   %lu ticks\r\n", memcpy_ticks);
    printf("%s", line);

    snprintf(line, sizeof(line),
             "2. DMA call blocked CPU for: %lu ticks\r\n", dma_call_blocked_ticks);
    printf("%s", line);

    snprintf(line, sizeof(line),
             "Actual hardware copy time:   %lu ticks\r\n", total_hardware_ticks);
    printf("%s", line);

    free(src);
    free(dst);
}

extern "C" void main_cpp()
{
    DWT_Init();
    dma_init();

    while(1) {
        run_benchmark();

        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(2000);
    }
}
