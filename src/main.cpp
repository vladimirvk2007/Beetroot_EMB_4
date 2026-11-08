#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pwm.h"

#define MOTOR_IN1 GPIO_NUM_16
#define MOTOR_IN2 GPIO_NUM_17

#define PWM_FREQUENCY_HZ 1000
#define PWM_RESOLUTION LEDC_TIMER_8_BIT
#define RAMP_DURATION_MS 5000
#define RAMP_STEPS 100
#define RAMP_STEP_DELAY_MS (RAMP_DURATION_MS / RAMP_STEPS)
#define STOP_TIME_MS 1000

static void ramp_motor(pwm_t *motor_pwm, bool accelerating) {
    for (int step = 0; step <= RAMP_STEPS; ++step) {
        const uint8_t percent = accelerating ? step : (RAMP_STEPS - step);
        ESP_ERROR_CHECK(pwm_set_percent(motor_pwm, percent));

        if (step < RAMP_STEPS) {
            vTaskDelay(pdMS_TO_TICKS(RAMP_STEP_DELAY_MS));
        }
    }
}

extern "C" void app_main() {
    pwm_t motor_in1_pwm = {};
    pwm_t motor_in2_pwm = {};

    pwm_config_t motor_in1_config = {
        .gpio = MOTOR_IN1,
        .channel = LEDC_CHANNEL_0,
        .timer = LEDC_TIMER_0,
        .frequency_hz = PWM_FREQUENCY_HZ,
        .resolution = PWM_RESOLUTION,
        .duty = 0,
        .inverted = false,
    };
    pwm_config_t motor_in2_config = {
        .gpio = MOTOR_IN2,
        .channel = LEDC_CHANNEL_1,
        .timer = LEDC_TIMER_0,
        .frequency_hz = PWM_FREQUENCY_HZ,
        .resolution = PWM_RESOLUTION,
        .duty = 0,
        .inverted = false,
    };

    ESP_ERROR_CHECK(pwm_init(&motor_in1_pwm, &motor_in1_config));
    ESP_ERROR_CHECK(pwm_init(&motor_in2_pwm, &motor_in2_config));

    printf("Motor control started: 5s ramp up, 5s ramp down, 1s coast stop.\n");

    while (1) {
        printf("[FORWARD] Ramping up...\n");
        ramp_motor(&motor_in1_pwm, true);
        printf("[FORWARD] Ramping down...\n");
        ramp_motor(&motor_in1_pwm, false);
        printf("[COAST] Stopped for 1 second.\n");
        vTaskDelay(pdMS_TO_TICKS(STOP_TIME_MS));

        printf("[REVERSE] Ramping up...\n");
        ramp_motor(&motor_in2_pwm, true);
        printf("[REVERSE] Ramping down...\n");
        ramp_motor(&motor_in2_pwm, false);
        printf("[COAST] Stopped for 1 second.\n");
        vTaskDelay(pdMS_TO_TICKS(STOP_TIME_MS));
    }
}
