#include <stdio.h>
#include "main.h"
#include "cmsis_os2.h"
#include "printf/usb_printf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

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

/* Черга копіює кожну команду цілком: номер LED та бажаний стан. */
struct LedCommand
{
    LedId led;
    GPIO_PinState state;
};

static QueueHandle_t led_queue;

/* Єдиний таск, який змінює стан світлодіодів після ініціалізації. */
static void queue_receiver_task(void *argument)
{
    (void)argument;
    LedCommand command;

    while (1)
    {
        /* Якщо черга порожня, таск чекає, не займаючи процесор. */
        if (xQueueReceive(led_queue, &command, portMAX_DELAY) != pdPASS)
        {
            printf("[ERROR] Failed to receive LED command\r\n");
            Error_Handler();
        }

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
            printf("[ERROR] Invalid LED in queue\r\n");
            Error_Handler();
            continue;
        }

        if (command.state != GPIO_PIN_SET && command.state != GPIO_PIN_RESET)
        {
            printf("[ERROR] Invalid LED state in queue\r\n");
            Error_Handler();
            continue;
        }

        HAL_GPIO_WritePin(LED_GPIO_PORT, pin, command.state);
        printf("[queueRx] LED%u %s\r\n", static_cast<unsigned int>(command.led),
               command.state == GPIO_PIN_SET ? "ON" : "OFF");
    }
}

static void send_led_command(LedId led, GPIO_PinState state)
{
    const LedCommand command = {led, state};

    /* Черга робить копію структури; якщо вона повна, чекаємо вільного місця. */
    if (xQueueSend(led_queue, &command, portMAX_DELAY) != pdPASS)
    {
        printf("[ERROR] Failed to send LED command\r\n");
        Error_Handler();
    }
}

static void queue_sender_task(void *argument)
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

    /* Черга вміщує чотири структури LedCommand і створюється до обох тасків. */
    led_queue = xQueueCreate(4U, sizeof(LedCommand));
    if (led_queue == nullptr)
    {
        printf("[ERROR] Failed to create LED queue\r\n");
        Error_Handler();
    }

    /* Розмір стеку xTaskCreate задано у словах, а не в байтах. */
    if (xTaskCreate(queue_receiver_task, "queueRx", 192U, nullptr,
                    tskIDLE_PRIORITY + 1U, nullptr) != pdPASS)
    {
        printf("[ERROR] Failed to create queue receiver task\r\n");
        Error_Handler();
    }
    if (xTaskCreate(queue_sender_task, "queueTx", 192U, nullptr,
                    tskIDLE_PRIORITY + 1U, nullptr) != pdPASS)
    {
        printf("[ERROR] Failed to create queue sender task\r\n");
        Error_Handler();
    }
}
