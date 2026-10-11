#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "uart/uart.h"

static UART_HandleTypeDef huart1;

static uint8_t received_byte;
static volatile bool rx_complete;

extern "C" void USART1_IRQHandler(void)
{
    UART_IRQHandler(&huart1);
}

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart != &huart1)
    {
        return;
    }

    rx_complete = true;
}

extern "C" void main_cpp()
{
    HAL_StatusTypeDef status = UART_Init(&huart1, USART1, 115200);
    if (status != HAL_OK)
    {
        printf("[UART] USART1 initialization failed (status=%d)\n", (int)status);
        Error_Handler();
    }

    printf("[UART] USART1 initialized: 115200 baud, 8N1\r\n");

    while (1)
    {
        rx_complete = false;

        status = HAL_UART_Receive_IT(&huart1, &received_byte, 1);
        if (status != HAL_OK)
        {
            printf("[UART] USART1 interrupt receive start failed (status=%d)\r\n",
                   (int)status);
            Error_Handler();
        }

        while (!rx_complete)
        {
        }

        status = UART_Transmit(&huart1, &received_byte, 1, 100);
        if (status != HAL_OK)
        {
            printf("[UART] Echo transmit failed (status=%d)\r\n", (int)status);
            Error_Handler();
        }
    }
}
