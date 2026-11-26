#include <stdio.h>
#include <string.h>
#include "main.h"
#include "cmsis_os2.h"
#include "printf/usb_printf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"

#define LED_GPIO_PORT GPIOA
#define LED_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
#define LED1_GPIO_PIN GPIO_PIN_1
#define LED2_GPIO_PIN GPIO_PIN_2
#define LED3_GPIO_PIN GPIO_PIN_3

static const size_t MAX_COMMAND_BYTES = 15U;
/* Із 80 байтів масиву один резервується реалізацією FreeRTOS. */
static uint8_t led_stream_storage[80U];
static StaticStreamBuffer_t led_stream_control;
static StreamBufferHandle_t led_stream_buffer;

static void apply_led_command(const char *command, size_t length)
{
    if (length < 5U || memcmp(command, "LED", 3U) != 0 || command[4] != ' ')
    {
        printf("[ERROR] Invalid LED command\r\n");
        Error_Handler();
        return;
    }

    uint16_t pin;
    switch (command[3])
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
        printf("[ERROR] Invalid LED in command\r\n");
        Error_Handler();
        return;
    }

    GPIO_PinState state;
    if (length == 7U && memcmp(&command[5], "ON", 2U) == 0)
    {
        state = GPIO_PIN_SET;
    }
    else if (length == 8U && memcmp(&command[5], "OFF", 3U) == 0)
    {
        state = GPIO_PIN_RESET;
    }
    else
    {
        printf("[ERROR] Invalid LED state in command\r\n");
        Error_Handler();
        return;
    }

    HAL_GPIO_WritePin(LED_GPIO_PORT, pin, state);
    printf("[streamRx] %s\r\n", command);
}

/* Єдиний отримувач збирає команди з порцій байтів до роздільника '\n'. */
static void stream_receiver_task(void *argument)
{
    (void)argument;
    char command[MAX_COMMAND_BYTES + 1U];
    size_t command_length = 0U;
    char bytes[3];

    while (1)
    {
        const size_t received = xStreamBufferReceive(
            led_stream_buffer, bytes, sizeof(bytes), portMAX_DELAY);
        if (received == 0U)
        {
            printf("[ERROR] Failed to receive LED stream\r\n");
            Error_Handler();
            continue;
        }

        printf("[streamRx] received %u bytes\r\n", static_cast<unsigned int>(received));
        for (size_t i = 0U; i < received; ++i)
        {
            if (bytes[i] == '\n')
            {
                command[command_length] = '\0';
                apply_led_command(command, command_length);
                command_length = 0U;
            }
            else if (command_length < MAX_COMMAND_BYTES)
            {
                command[command_length++] = bytes[i];
            }
            else
            {
                printf("[ERROR] LED command is too long\r\n");
                Error_Handler();
            }
        }
    }
}

static void send_led_stream(const char *command)
{
    const size_t length = strlen(command);
    size_t sent = 0U;

    /* Stream Buffer може записати лише частину переданих байтів. */
    while (sent < length)
    {
        const size_t written = xStreamBufferSend(
            led_stream_buffer, command + sent, length - sent, portMAX_DELAY);
        if (written == 0U)
        {
            printf("[ERROR] Failed to send LED stream\r\n");
            Error_Handler();
            return;
        }
        sent += written;
    }
}

static void stream_sender_task(void *argument)
{
    (void)argument;

    while (1)
    {
        /* Після кожної команди таск віддає процесор на 1000 тактів (1 с). */
        send_led_stream("LED1 ON\n");
        osDelay(1000U);
        send_led_stream("LED2 ON\n");
        osDelay(1000U);
        send_led_stream("LED3 ON\n");
        osDelay(1000U);
        send_led_stream("LED3 OFF\n");
        osDelay(1000U);
        send_led_stream("LED2 OFF\n");
        osDelay(1000U);
        send_led_stream("LED1 OFF\n");
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

    led_stream_buffer = xStreamBufferCreateStatic(
        sizeof(led_stream_storage), 1U, led_stream_storage, &led_stream_control);
    if (led_stream_buffer == nullptr)
    {
        printf("[ERROR] Failed to create LED stream buffer\r\n");
        Error_Handler();
    }

    /* Розмір стеку xTaskCreate задано у словах, а не в байтах. */
    if (xTaskCreate(stream_receiver_task, "streamRx", 192U, nullptr,
                    tskIDLE_PRIORITY + 1U, nullptr) != pdPASS)
    {
        printf("[ERROR] Failed to create stream receiver task\r\n");
        Error_Handler();
    }
    if (xTaskCreate(stream_sender_task, "streamTx", 192U, nullptr,
                    tskIDLE_PRIORITY + 1U, nullptr) != pdPASS)
    {
        printf("[ERROR] Failed to create stream sender task\r\n");
        Error_Handler();
    }
}
