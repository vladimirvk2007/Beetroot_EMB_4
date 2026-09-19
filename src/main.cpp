#include <stdio.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pwm.h"
#include "sound.h"

static const char *TAG = "main";

extern "C" void app_main(void) {
    esp_err_t last_error = ESP_OK;

    // 1. Конфігурація PWM для бузера/динаміка
    pwm_t buzzer_pwm = {};
    pwm_config_t buzzer_cfg = {
        .gpio = GPIO_NUM_15,
        .channel = LEDC_CHANNEL_0,
        .timer = LEDC_TIMER_0,
        .frequency_hz = 10000,
        .resolution = LEDC_TIMER_12_BIT,
        .duty = 0,
        .inverted = false
    };

    esp_err_t ret = pwm_init(&buzzer_pwm, &buzzer_cfg);
    if (ret != ESP_OK) {
        last_error = ret;
        ESP_LOGE(TAG, "pwm_init failed: %s", esp_err_to_name(ret));
    }

    // 2. Створення звукового генератора
    sound_t tone = {};
    ret = sound_init(&tone, &buzzer_pwm, 3400);
    if (ret != ESP_OK) {
        last_error = ret;
        ESP_LOGE(TAG, "sound_init failed: %s", esp_err_to_name(ret));
        pwm_deinit(&buzzer_pwm);
    } else {
        // 3. Запуск звуку
        ret = sound_start(&tone);
        if (ret != ESP_OK) {
            last_error = ret;
            ESP_LOGE(TAG, "sound_start failed: %s", esp_err_to_name(ret));
            sound_deinit(&tone);
            pwm_deinit(&buzzer_pwm);
        }
    }

    while (1) {
        if (last_error != ESP_OK) {
            printf("Last init error: %s\n", esp_err_to_name(last_error));
        } else {
            printf("Running...\n");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
