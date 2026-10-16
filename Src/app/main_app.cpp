#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "uart/uart.h"

static UART_HandleTypeDef huart1;

static uint8_t rx_byte;
static uint8_t tx_byte;
static volatile uint8_t rx_done = 0;
static volatile uint8_t tx_busy = 0;

static void on_rx_complete(UART_HandleTypeDef *huart)
{
    (void)huart;
    rx_done = 1;
}

static void on_tx_complete(UART_HandleTypeDef *huart)
{
    (void)huart;
    tx_busy = 0;
}

// Ехо через DMA: прийом і передача виконуються без участі CPU
extern "C" void main_cpp()
{
    HAL_StatusTypeDef status = UART_Init(&huart1, USART1, 115200);
    if (status != HAL_OK)
    {
        printf("[UART] USART1 initialization failed (status=%d)\n", (int)status);
        Error_Handler();
    }

    printf("[UART] USART1 initialized: 115200 baud, 8N1, DMA\n");

    UART_RegisterDMACallbacks(on_tx_complete, on_rx_complete);

    status = UART_Receive_DMA(&huart1, &rx_byte, 1);
    if (status != HAL_OK)
    {
        printf("[UART] DMA receive start failed (status=%d)\n", (int)status);
        Error_Handler();
    }

    while (1)
    {
        if (!rx_done)
        {
            continue;
        }
        rx_done = 0;

        tx_byte = rx_byte;

        // Одразу перезапускаємо прийом, щоб не втратити наступний байт
        status = UART_Receive_DMA(&huart1, &rx_byte, 1);
        if (status != HAL_OK)
        {
            printf("[UART] DMA receive restart failed (status=%d)\n", (int)status);
            Error_Handler();
        }

        printf("[UART] RX: 0x%02X - '%c'\n", tx_byte, tx_byte);

        tx_busy = 1;
        status = UART_Transmit_DMA(&huart1, &tx_byte, 1);
        if (status != HAL_OK)
        {
            printf("[UART] DMA echo transmit failed (status=%d)\n", (int)status);
            Error_Handler();
        }

        // Поки йде передача, CPU вільний: блимаємо світлодіодом
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        while (tx_busy)
        {
            __NOP();
        }
    }
}
