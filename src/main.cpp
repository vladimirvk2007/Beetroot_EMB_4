#include <stdio.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pwm.h"
#include "servo.h"

#define SERVO_GPIO GPIO_NUM_15
#define SERVO_INITIAL_ANGLE 90
#define SERVO_MOVE_STEP 10

static const char *TAG = "main";

extern "C" void app_main(void) {
    esp_err_t last_error = ESP_OK;

    // 1. Конфігурація PWM для SG90
    pwm_t servo_pwm = {};
    pwm_config_t servo_cfg = {
        .gpio = SERVO_GPIO,
        .channel = LEDC_CHANNEL_0,
        .timer = LEDC_TIMER_0,
        .frequency_hz = SERVO_FREQUENCY_HZ,
        .resolution = LEDC_TIMER_14_BIT,
        .duty = 0,
        .inverted = false
    };

    esp_err_t ret = pwm_init(&servo_pwm, &servo_cfg);
    if (ret != ESP_OK) {
        last_error = ret;
        ESP_LOGE(TAG, "pwm_init failed: %s", esp_err_to_name(ret));
    }

    // 2. Ініціалізація сервомотора
    servo_t servo = {};
    ret = servo_init(&servo, &servo_pwm);
    if (ret != ESP_OK) {
        last_error = ret;
        ESP_LOGE(TAG, "servo_init failed: %s", esp_err_to_name(ret));
        pwm_deinit(&servo_pwm);
    } else {
        // Початкове положення сервомотора.
        ret = servo_set_angle(&servo, SERVO_INITIAL_ANGLE);
        if (ret != ESP_OK) {
            last_error = ret;
            ESP_LOGE(TAG, "servo_set_angle failed: %s", esp_err_to_name(ret));
        }
    }

    uint16_t angle = SERVO_INITIAL_ANGLE;
    while (1) {
        if (last_error != ESP_OK) {
            printf("Last init error: %s\n", esp_err_to_name(last_error));
        } else {
            ret = servo_set_angle(&servo, angle);
            if (ret != ESP_OK) {
                last_error = ret;
                ESP_LOGE(TAG, "servo_set_angle failed: %s", esp_err_to_name(ret));
            } else {
                printf("Servo angle: %u degrees\n", angle);
                if (angle >= SERVO_MAX_ANGLE) {
                    angle = SERVO_MIN_ANGLE;
                } else {
                    angle += SERVO_MOVE_STEP;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
