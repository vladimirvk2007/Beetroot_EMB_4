#include <stdio.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pwm.h"

#define LED_OUT GPIO_NUM_16

extern "C" void app_main() {
    pwm_t led_pwm = {};
    pwm_config_t led_cfg = {
        .gpio = LED_OUT,
        .channel = LEDC_CHANNEL_0,
        .timer = LEDC_TIMER_0,
        .frequency_hz = 1000,
        .resolution = LEDC_TIMER_8_BIT,
        .duty = 0,
        .inverted = false,
    };

    ESP_ERROR_CHECK(pwm_init(&led_pwm, &led_cfg));

    while (1) {
        for (int percent = 0; percent <= 100; percent += 10) {
            ESP_ERROR_CHECK(pwm_set_percent(&led_pwm, (uint8_t)percent));
            printf("PWM check: %d%%\n", percent);
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }

        for (int percent = 100; percent >= 0; percent -= 10) {
            ESP_ERROR_CHECK(pwm_set_percent(&led_pwm, (uint8_t)percent));
            printf("PWM check: %d%%\n", percent);
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }
    }
}
