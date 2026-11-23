#include <stdlib.h>

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_1_PIN GPIO_NUM_16
#define LED_2_PIN GPIO_NUM_17
#define LED_3_PIN GPIO_NUM_18
#define LED_TASK_STACK_SIZE 2048U
#define LED_TASK_PRIORITY 5U

static const char *TAG = "LED_DEMO";

struct LedTaskConfig
{
    gpio_num_t pin;
    uint32_t delay_ms;
};

static LedTaskConfig led_configs[] = {
    {LED_1_PIN, 500U},
    {LED_2_PIN, 1000U},
    {LED_3_PIN, 1500U},
};

static void led_task(void *argument)
{
    const LedTaskConfig *config = static_cast<const LedTaskConfig *>(argument);
    bool led_on = false;

    while (true)
    {
        led_on = !led_on;
        /* Світлодіоди вмикаються низьким рівнем. */
        ESP_ERROR_CHECK(gpio_set_level(config->pin, led_on ? 0 : 1));
        vTaskDelay(pdMS_TO_TICKS(config->delay_ms));
    }
}

extern "C" void app_main()
{
    gpio_config_t gpio = {};
    gpio.pin_bit_mask = (1ULL << LED_1_PIN) | (1ULL << LED_2_PIN) | (1ULL << LED_3_PIN);
    gpio.mode = GPIO_MODE_OUTPUT;
    gpio.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio.intr_type = GPIO_INTR_DISABLE;

    /* Встановлюємо неактивний рівень перед увімкненням виходів. */
    ESP_ERROR_CHECK(gpio_set_level(LED_1_PIN, 1));
    ESP_ERROR_CHECK(gpio_set_level(LED_2_PIN, 1));
    ESP_ERROR_CHECK(gpio_set_level(LED_3_PIN, 1));
    ESP_ERROR_CHECK(gpio_config(&gpio));

    /* Кожна таска отримує власний пін і затримку. */
    for (unsigned int i = 0; i < sizeof(led_configs) / sizeof(led_configs[0]); ++i)
    {
        if (xTaskCreate(led_task, "ledTask", LED_TASK_STACK_SIZE, &led_configs[i],
                        LED_TASK_PRIORITY, nullptr) != pdPASS)
        {
            ESP_LOGE(TAG, "Failed to create LED task for GPIO %d", led_configs[i].pin);
            abort();
        }
    }
}
