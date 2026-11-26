#include <stdio.h>
#include "main.h"
#include "cmsis_os2.h"
#include "printf/usb_printf.h"
#include "FreeRTOS.h"
#include "task.h"

#define LED_GPIO_PORT GPIOA
#define LED_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
#define LED1_GPIO_PIN GPIO_PIN_1
#define LED2_GPIO_PIN GPIO_PIN_2
#define LED3_GPIO_PIN GPIO_PIN_3

/* Кожна команда займає окремий біт 32-бітного значення notification. */
static const uint32_t NOTIFY_LED1_ON = (1U << 0);
static const uint32_t NOTIFY_LED1_OFF = (1U << 1);
static const uint32_t NOTIFY_LED2_ON = (1U << 2);
static const uint32_t NOTIFY_LED2_OFF = (1U << 3);
static const uint32_t NOTIFY_LED3_ON = (1U << 4);
static const uint32_t NOTIFY_LED3_OFF = (1U << 5);
static const uint32_t NOTIFY_LED_MASK = NOTIFY_LED1_ON | NOTIFY_LED1_OFF |
                                        NOTIFY_LED2_ON | NOTIFY_LED2_OFF |
                                        NOTIFY_LED3_ON | NOTIFY_LED3_OFF;

static TaskHandle_t notification_receiver_handle;

/* Єдиний таск, який змінює стан світлодіодів після ініціалізації. */
static void notification_receiver_task(void *argument)
{
    (void)argument;
    uint32_t notification_bits;

    while (1)
    {
        /* На вході біти не очищуються; на виході маска копіюється у
           notification_bits, а біти команд очищуються в notification таску. */
        if (xTaskNotifyWait(0U, NOTIFY_LED_MASK, &notification_bits, portMAX_DELAY) == pdPASS)
        {
            /* Кілька команд можуть об'єднатися: перевіряємо кожен біт окремо.
               Для одного LED біт OFF обробляється після біта ON. */
            for (uint32_t bit = NOTIFY_LED1_ON; bit <= NOTIFY_LED3_OFF; bit <<= 1U)
            {
                switch (notification_bits & bit)
                {
                case NOTIFY_LED1_ON:
                    HAL_GPIO_WritePin(LED_GPIO_PORT, LED1_GPIO_PIN, GPIO_PIN_SET);
                    printf("[notifyRx] LED1 ON\r\n");
                    break;
                case NOTIFY_LED1_OFF:
                    HAL_GPIO_WritePin(LED_GPIO_PORT, LED1_GPIO_PIN, GPIO_PIN_RESET);
                    printf("[notifyRx] LED1 OFF\r\n");
                    break;
                case NOTIFY_LED2_ON:
                    HAL_GPIO_WritePin(LED_GPIO_PORT, LED2_GPIO_PIN, GPIO_PIN_SET);
                    printf("[notifyRx] LED2 ON\r\n");
                    break;
                case NOTIFY_LED2_OFF:
                    HAL_GPIO_WritePin(LED_GPIO_PORT, LED2_GPIO_PIN, GPIO_PIN_RESET);
                    printf("[notifyRx] LED2 OFF\r\n");
                    break;
                case NOTIFY_LED3_ON:
                    HAL_GPIO_WritePin(LED_GPIO_PORT, LED3_GPIO_PIN, GPIO_PIN_SET);
                    printf("[notifyRx] LED3 ON\r\n");
                    break;
                case NOTIFY_LED3_OFF:
                    HAL_GPIO_WritePin(LED_GPIO_PORT, LED3_GPIO_PIN, GPIO_PIN_RESET);
                    printf("[notifyRx] LED3 OFF\r\n");
                    break;
                default:
                    break;
                }
            }
        }
    }
}

static void send_led_notification(uint32_t notification_bit)
{
    /* eSetBits додає команду через OR, не стираючи інші очікувані біти. */
    if (xTaskNotify(notification_receiver_handle, notification_bit, eSetBits) != pdPASS)
    {
        printf("[ERROR] Failed to send LED notification\r\n");
        Error_Handler();
    }
}

static void notification_sender_task(void *argument)
{
    (void)argument;

    while (1)
    {
        /* Після кожної команди таск віддає процесор на 1000 тактів (1 с). */
        send_led_notification(NOTIFY_LED1_ON);
        osDelay(1000U);
        send_led_notification(NOTIFY_LED2_ON);
        osDelay(1000U);
        send_led_notification(NOTIFY_LED3_ON);
        osDelay(1000U);
        send_led_notification(NOTIFY_LED3_OFF);
        osDelay(1000U);
        send_led_notification(NOTIFY_LED2_OFF);
        osDelay(1000U);
        send_led_notification(NOTIFY_LED1_OFF);
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

    /* Створюємо отримувача першим, щоб його handle був готовий для відправника.
       Розмір стеку xTaskCreate задано у словах, а не в байтах. */
    if (xTaskCreate(notification_receiver_task, "notifyRx", 192U, nullptr,
                    tskIDLE_PRIORITY + 1U, &notification_receiver_handle) != pdPASS)
    {
        printf("[ERROR] Failed to create notification receiver task\r\n");
        Error_Handler();
    }
    if (xTaskCreate(notification_sender_task, "notifyTx", 192U, nullptr,
                    tskIDLE_PRIORITY + 1U, nullptr) != pdPASS)
    {
        printf("[ERROR] Failed to create notification sender task\r\n");
        Error_Handler();
    }
}
