#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "drv8833.h"

// Виводи керування драйвером двигуна.
#define MOTOR_IN1 GPIO_NUM_16
#define MOTOR_IN2 GPIO_NUM_17

// Параметри PWM і тривалість етапів циклу.
#define PWM_FREQUENCY_HZ 1000
#define PWM_RESOLUTION LEDC_TIMER_8_BIT
#define RAMP_DURATION_MS 5000
#define RAMP_STEPS 100
#define RAMP_STEP_DELAY_MS (RAMP_DURATION_MS / RAMP_STEPS)
#define STOP_TIME_MS 1000

// Плавно змінює швидкість двигуна в заданому напрямку.
static void ramp_motor(drv8833_t *motor, int16_t direction, bool accelerating) {
    for (int step = 0; step <= RAMP_STEPS; ++step) {
        const int16_t magnitude = accelerating ? step : (RAMP_STEPS - step);
        ESP_ERROR_CHECK(drv8833_set_speed(motor, direction * magnitude));

        if (step < RAMP_STEPS) {
            vTaskDelay(pdMS_TO_TICKS(RAMP_STEP_DELAY_MS));
        }
    }
}

extern "C" void app_main() {
    drv8833_t motor = {};
    drv8833_config_t motor_config = {
        .in1 = {
            .gpio = MOTOR_IN1,
            .channel = LEDC_CHANNEL_0,
            .timer = LEDC_TIMER_0,
            .frequency_hz = PWM_FREQUENCY_HZ,
            .resolution = PWM_RESOLUTION,
            .duty = 0,
            .inverted = false,
        },
        .in2 = {
            .gpio = MOTOR_IN2,
            .channel = LEDC_CHANNEL_1,
            .timer = LEDC_TIMER_0,
            .frequency_hz = PWM_FREQUENCY_HZ,
            .resolution = PWM_RESOLUTION,
            .duty = 0,
            .inverted = false,
        },
    };

    ESP_ERROR_CHECK(drv8833_init(&motor, &motor_config));

    printf("Motor control: 5s ramp up, 5s ramp down, 1s coast stop.\n");

    while (1) {
        printf("[FORWARD] Ramping up...\n");
        ramp_motor(&motor, 1, true);
        printf("[FORWARD] Ramping down...\n");
        ramp_motor(&motor, 1, false);
        printf("[COAST] Stopped for 1 second.\n");
        vTaskDelay(pdMS_TO_TICKS(STOP_TIME_MS));

        printf("[REVERSE] Ramping up...\n");
        ramp_motor(&motor, -1, true);
        printf("[REVERSE] Ramping down...\n");
        ramp_motor(&motor, -1, false);
        printf("[COAST] Stopped for 1 second.\n");
        vTaskDelay(pdMS_TO_TICKS(STOP_TIME_MS));
    }
}
