#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"

#define LED_OUT		GPIO_NUM_16
#define BUTTON_IN	GPIO_NUM_15


static bool IRAM_ATTR timer_on_alarm_cb(gptimer_handle_t timer,
                                         const gptimer_alarm_event_data_t *edata,
                                         void *user_data) {
    static bool led_state = 0;

    gpio_set_level(LED_OUT, !led_state);
    led_state = !led_state;

    return true;
}

extern "C" void app_main() {

    esp_err_t err = ESP_OK;

    // Налаштування структури GPIO для LED
    gpio_config_t gpio_led_conf = {};
    gpio_led_conf.pin_bit_mask = 1ULL << LED_OUT;
    gpio_led_conf.mode = GPIO_MODE_OUTPUT;
    gpio_led_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_led_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_led_conf.intr_type = GPIO_INTR_DISABLE;

    // Налаштування структури GPIO для BUTTON
    gpio_config_t gpio_button_conf = {};
    gpio_button_conf.pin_bit_mask = 1ULL << BUTTON_IN;
    gpio_button_conf.mode = GPIO_MODE_INPUT;
    gpio_button_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_button_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_button_conf.intr_type = GPIO_INTR_DISABLE;

    // Об'єкт таймера
    gptimer_handle_t timer;
    // Конфігурація таймера
    gptimer_config_t timer_config = {};
    timer_config.clk_src = GPTIMER_CLK_SRC_DEFAULT;
    timer_config.direction = GPTIMER_COUNT_UP;
    timer_config.resolution_hz = 1000000; // 1 us per tick
    // Створення нового таймера
    err = gptimer_new_timer(&timer_config, &timer);
    if (err != ESP_OK) {
        printf("Failed to create timer, err = %d\n", err);
        return;
    }

    // Конфігурація аларму таймера
    gptimer_alarm_config_t alarm_config = {};
    alarm_config.alarm_count = 1000000; // Тривалість 1 с
    alarm_config.reload_count = 0;
    alarm_config.flags.auto_reload_on_alarm = true;

    // Створення аларму таймера
    gptimer_set_alarm_action(timer, &alarm_config);

    gptimer_event_callbacks_t timer_callbacks = {};
    timer_callbacks.on_alarm = timer_on_alarm_cb;

    gptimer_register_event_callbacks(timer, &timer_callbacks, NULL);

    // Конфігурація GPIO
    gpio_config(&gpio_led_conf);
    gpio_config(&gpio_button_conf);

    // Встановлення початкового стану
    gpio_set_level(LED_OUT, 0);

    gptimer_enable(timer);
    gptimer_start(timer);

    while (1) {
        //bool btn_state = gpio_get_level(BUTTON_IN);

        //gpio_set_level(LED_OUT, !btn_state);

        vTaskDelay(pdMS_TO_TICKS(100));
        bool led_state = gpio_get_level(LED_OUT);
        printf("LED state: %d\n", led_state);
    }
}
