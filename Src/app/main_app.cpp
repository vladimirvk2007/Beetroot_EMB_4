#include <stdio.h>
#include "main.h"
#include "cmsis_os2.h"
#include "printf/usb_printf.h"
#include "printf/usb_printf.h"

#define LED_GPIO_PORT GPIOA
#define LED_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
#define LED1_GPIO_PIN GPIO_PIN_0
#define LED2_GPIO_PIN GPIO_PIN_1
#define LED3_GPIO_PIN GPIO_PIN_2

struct LedTaskConfig
{
    uint16_t pin;
    uint32_t delay_ms;
};

static LedTaskConfig led_configs[] = {
    {LED1_GPIO_PIN, 500U},
    {LED2_GPIO_PIN, 1000U},
    {LED3_GPIO_PIN, 1500U},
};

static void led_task(void *argument)
{
    const LedTaskConfig *config = static_cast<const LedTaskConfig *>(argument);
    GPIO_PinState state = GPIO_PIN_RESET;

    while (1)
    {
        state = (state == GPIO_PIN_RESET) ? GPIO_PIN_SET : GPIO_PIN_RESET;
        HAL_GPIO_WritePin(LED_GPIO_PORT, config->pin, state);
        osDelay(config->delay_ms);
    }
}

extern "C" void main_cpp()
{
    LED_GPIO_CLK_ENABLE();
    HAL_GPIO_WritePin(LED_GPIO_PORT, LED1_GPIO_PIN | LED2_GPIO_PIN | LED3_GPIO_PIN,
                      GPIO_PIN_RESET);

    GPIO_InitTypeDef gpio = {};
    gpio.Pin = LED1_GPIO_PIN | LED2_GPIO_PIN | LED3_GPIO_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_PORT, &gpio);

    const osThreadAttr_t attributes = {
        "ledTask", 0U, nullptr, 0U, nullptr, 512U, osPriorityNormal, 0U, 0U
    };

    /* Кожна таска керує власним піном і віддає процесор під час затримки. */
    for (unsigned int i = 0; i < sizeof(led_configs) / sizeof(led_configs[0]); ++i)
    {
        if (osThreadNew(led_task, &led_configs[i], &attributes) == nullptr)
        {
            printf("[ERROR] Failed to create LED task\r\n");
            Error_Handler();
        }
    }
}
