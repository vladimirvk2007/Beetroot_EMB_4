#include <stdio.h>
#include "main.h"
#include "cmsis_os2.h"
#include "printf/usb_printf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define LED_GPIO_PORT GPIOA
#define LED_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
#define LED1_GPIO_PIN GPIO_PIN_1
#define LED2_GPIO_PIN GPIO_PIN_2
#define LED3_GPIO_PIN GPIO_PIN_3

enum LedId
{
    LED1 = 1,
    LED2,
    LED3
};

struct LedCommand
{
    LedId led;
    GPIO_PinState state;
};

/* Спільна команда передається через пам'ять, а семафор сигналізує про запис. */
static LedCommand shared_command;
static SemaphoreHandle_t command_semaphore;

static void led_receiver_task(void *argument)
{
    (void)argument;

    while (1)
    {
        if (xSemaphoreTake(command_semaphore, portMAX_DELAY) != pdTRUE)
        {
            printf("[ERROR] Failed to wait for LED command\r\n");
            Error_Handler();
        }
        taskENTER_CRITICAL();
        const LedCommand command = shared_command;
        taskEXIT_CRITICAL();

        uint16_t pin;
        switch (command.led)
        {
        case LED1:
            pin = LED1_GPIO_PIN;
            break;
        case LED2:
            pin = LED2_GPIO_PIN;
            break;
        case LED3:
            pin = LED3_GPIO_PIN;
            break;
        default:
            printf("[ERROR] Invalid LED in command\r\n");
            Error_Handler();
            continue;
        }
        if (command.state != GPIO_PIN_SET && command.state != GPIO_PIN_RESET)
        {
            printf("[ERROR] Invalid LED state\r\n");
            Error_Handler();
            continue;
        }

        HAL_GPIO_WritePin(LED_GPIO_PORT, pin, command.state);
        printf("[ledRx] LED%u %s\r\n", static_cast<unsigned int>(command.led),
               command.state == GPIO_PIN_SET ? "ON" : "OFF");
    }
}

static void send_led_command(LedId led, GPIO_PinState state)
{
    taskENTER_CRITICAL();
    shared_command = {led, state};
    taskEXIT_CRITICAL();

    if (xSemaphoreGive(command_semaphore) != pdTRUE)
    {
        printf("[ledTx] Command semaphore is full\r\n");
    }
}

static void led_sender_task(void *argument)
{
    (void)argument;

    while (1)
    {
        /* Після кожної команди таск віддає процесор на 1000 тактів (1 с). */
        send_led_command(LED1, GPIO_PIN_SET);
        osDelay(1000U);
        send_led_command(LED2, GPIO_PIN_SET);
        osDelay(1000U);
        send_led_command(LED3, GPIO_PIN_SET);
        osDelay(1000U);
        send_led_command(LED3, GPIO_PIN_RESET);
        osDelay(1000U);
        send_led_command(LED2, GPIO_PIN_RESET);
        osDelay(1000U);
        send_led_command(LED1, GPIO_PIN_RESET);
        osDelay(1000U);
    }
}

extern "C" void main_cpp()
{
    LED_GPIO_CLK_ENABLE();
    /* Перед налаштуванням виходів початковий стан усіх LED — вимкнено. */
    HAL_GPIO_WritePin(LED_GPIO_PORT, LED1_GPIO_PIN | LED2_GPIO_PIN | LED3_GPIO_PIN,
                      GPIO_PIN_RESET);

    GPIO_InitTypeDef gpio = {};
    gpio.Pin = LED1_GPIO_PIN | LED2_GPIO_PIN | LED3_GPIO_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_PORT, &gpio);

    /* Лічильник семафора: максимум 3 сигнали, початкове значення 0. */
    command_semaphore = xSemaphoreCreateCounting(3U, 0U);
    if (command_semaphore == nullptr)
    {
        printf("[ERROR] Failed to create command semaphore\r\n");
        Error_Handler();
    }
    /* Розмір стеку xTaskCreate задано у словах, а не в байтах. */
    if (xTaskCreate(led_receiver_task, "ledRx", 192U, nullptr,
                    tskIDLE_PRIORITY + 1U, nullptr) != pdPASS)
    {
        printf("[ERROR] Failed to create LED receiver task\r\n");
        Error_Handler();
    }
    if (xTaskCreate(led_sender_task, "ledTx", 192U, nullptr,
                    tskIDLE_PRIORITY + 1U, nullptr) != pdPASS)
    {
        printf("[ERROR] Failed to create LED sender task\r\n");
        Error_Handler();
    }
}
