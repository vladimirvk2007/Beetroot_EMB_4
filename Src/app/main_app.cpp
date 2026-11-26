#include <stdio.h>
#include <string.h>
#include "main.h"
#include "cmsis_os2.h"
#include "printf/usb_printf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

#define LED_GPIO_PORT GPIOA
#define LED_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
#define LED1_GPIO_PIN GPIO_PIN_1
#define LED2_GPIO_PIN GPIO_PIN_2
#define LED3_GPIO_PIN GPIO_PIN_3

static const size_t MAX_MESSAGE_BYTES = 15U;
/* Із 80 байтів масиву один резервується реалізацією FreeRTOS. */
static uint8_t led_message_storage[80U];
static StaticMessageBuffer_t led_message_control;
static MessageBufferHandle_t led_message_buffer;

/* Єдиний таск, який змінює стан світлодіодів після ініціалізації. */
static void message_receiver_task(void *argument)
{
    (void)argument;
    char message[MAX_MESSAGE_BYTES + 1U];

    while (1)
    {
        /* Отримуємо одне повне повідомлення; залишаємо місце для '\0'. */
        const size_t received = xMessageBufferReceive(
            led_message_buffer, message, MAX_MESSAGE_BYTES, portMAX_DELAY);
        if (received == 0U)
        {
            printf("[ERROR] Failed to receive LED message\r\n");
            Error_Handler();
            continue;
        }
        message[received] = '\0';

        if (received < 5U || memcmp(message, "LED", 3U) != 0 || message[4] != ' ')
        {
            printf("[ERROR] Invalid LED message\r\n");
            Error_Handler();
            continue;
        }

        uint16_t pin;
        switch (message[3])
        {
        case '1':
            pin = LED1_GPIO_PIN;
            break;
        case '2':
            pin = LED2_GPIO_PIN;
            break;
        case '3':
            pin = LED3_GPIO_PIN;
            break;
        default:
            printf("[ERROR] Invalid LED in message\r\n");
            Error_Handler();
            continue;
        }

        GPIO_PinState state;
        if (received == 7U && memcmp(&message[5], "ON", 2U) == 0)
        {
            state = GPIO_PIN_SET;
        }
        else if (received == 8U && memcmp(&message[5], "OFF", 3U) == 0)
        {
            state = GPIO_PIN_RESET;
        }
        else
        {
            printf("[ERROR] Invalid LED state in message\r\n");
            Error_Handler();
            continue;
        }

        HAL_GPIO_WritePin(LED_GPIO_PORT, pin, state);
        printf("[messageRx] %s (%u bytes)\r\n", message, static_cast<unsigned int>(received));
    }
}

static void send_led_message(const char *message)
{
    const size_t length = strlen(message);
    if (length == 0U || length > MAX_MESSAGE_BYTES)
    {
        printf("[ERROR] Invalid LED message length\r\n");
        Error_Handler();
        return;
    }

    /* Передаємо текст без '\0'; message buffer зберігає його довжину. */
    if (xMessageBufferSend(led_message_buffer, message, length, portMAX_DELAY) != length)
    {
        printf("[ERROR] Failed to send LED message\r\n");
        Error_Handler();
    }
}

static void message_sender_task(void *argument)
{
    (void)argument;

    while (1)
    {
        /* Після кожної команди таск віддає процесор на 1000 тактів (1 с). */
        send_led_message("LED1 ON");
        osDelay(1000U);
        send_led_message("LED2 ON");
        osDelay(1000U);
        send_led_message("LED3 ON");
        osDelay(1000U);
        send_led_message("LED3 OFF");
        osDelay(1000U);
        send_led_message("LED2 OFF");
        osDelay(1000U);
        send_led_message("LED1 OFF");
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

    led_message_buffer = xMessageBufferCreateStatic(
        sizeof(led_message_storage), led_message_storage, &led_message_control);
    if (led_message_buffer == nullptr)
    {
        printf("[ERROR] Failed to create LED message buffer\r\n");
        Error_Handler();
    }

    /* Розмір стеку xTaskCreate задано у словах, а не в байтах. */
    if (xTaskCreate(message_receiver_task, "messageRx", 192U, nullptr,
                    tskIDLE_PRIORITY + 1U, nullptr) != pdPASS)
    {
        printf("[ERROR] Failed to create message receiver task\r\n");
        Error_Handler();
    }
    if (xTaskCreate(message_sender_task, "messageTx", 192U, nullptr,
                    tskIDLE_PRIORITY + 1U, nullptr) != pdPASS)
    {
        printf("[ERROR] Failed to create message sender task\r\n");
        Error_Handler();
    }
}
