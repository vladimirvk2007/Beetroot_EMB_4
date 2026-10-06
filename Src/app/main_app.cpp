#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "uart/uart.h"

static UART_HandleTypeDef huart1;

extern "C" void main_cpp()
{
    uint8_t received_byte = 0;

    HAL_StatusTypeDef status = UART_Init(&huart1, USART1, 115200);
    if (status != HAL_OK)
    {
        printf("[UART] USART1 initialization failed (status=%d)\n", (int)status);
        Error_Handler();
    }

    printf("[UART] USART1 initialized: 115200 baud, 8N1\n");

    while (1)
    {
        status = UART_Receive(&huart1, &received_byte, 1, 0);
        if (status == HAL_OK)
        {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
            HAL_Delay(50);
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

            printf("[UART] RX: 0x%02X - \"%s\"\n", received_byte, &received_byte);

            status = UART_Transmit(&huart1, &received_byte, 1, 100);
            if (status != HAL_OK)
            {
                printf("[UART] Echo transmit failed (status=%d)\r\n", (int)status);
                Error_Handler();
            }
        }
        else if (status != HAL_TIMEOUT)
        {
            printf("[UART] Receive failed (status=%d)\r\n", (int)status);
            Error_Handler();
        }

        HAL_Delay(10);
    }
}
