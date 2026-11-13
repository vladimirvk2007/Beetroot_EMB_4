#include <math.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "drv8833.h"
#include "encoder.h"
#include "pid.h"

// Драйвер DRV8833.
#define MOTOR_IN1 GPIO_NUM_16
#define MOTOR_IN2 GPIO_NUM_17

// Канали KY-040 та калібрування енкодера.
#define ENCODER_A GPIO_NUM_7
#define ENCODER_B GPIO_NUM_15
#define ENCODER_COUNTS_PER_REVOLUTION 80.0f
#define ENCODER_DIRECTION 1.0f
#define ENCODER_GLITCH_FILTER_NS 10000

// Параметри приводу та PID-регулятора.
#define PWM_FREQUENCY_HZ 1000
#define PWM_RESOLUTION LEDC_TIMER_8_BIT
#define ANGLE_TOLERANCE_DEG 3.0f
#define CONTROL_PERIOD_MS 20
#define LOG_PERIOD_MS 250
#define SETPOINT_INTERVAL_MS 20000

// Уставки абсолютного кута відносно початкового положення двигуна.
static const float ANGLE_SETPOINTS_DEG[] = {
    90.0f, 75.0f, 115.0f, 50.0f, 150.0f, 25.0f, 200.0f, 9.0f, 270.0f,
};
#define SETPOINT_COUNT (sizeof(ANGLE_SETPOINTS_DEG) / sizeof(ANGLE_SETPOINTS_DEG[0]))

// Під час руху додатна команда відповідає руху в напрямку IN1.
static const pid_config_t PID_CONFIG = {
    .kp = 2.0f,
    .ki = 0.02f,
    .kd = 0.1f,
    .output_min = -100.0f,
    .output_max = 100.0f,
    .integral_min = -500.0f,
    .integral_max = 500.0f,
};

extern "C" void app_main() {
    drv8833_t motor = {};
    const drv8833_config_t motor_config = {
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

    encoder_ctx_t encoder = {};
    ESP_ERROR_CHECK(encoder_init(&encoder,
                                 ENCODER_A,
                                 ENCODER_B,
                                 GPIO_NUM_NC,
                                 ENCODER_GLITCH_FILTER_NS));

    pid_controller_t angle_pid = {};
    ESP_ERROR_CHECK(pid_init(&angle_pid, &PID_CONFIG));

    int32_t initial_count = 0;
    ESP_ERROR_CHECK(encoder_read(&encoder, &initial_count));

    printf("Starting angle target: %.1f deg; encoder: %.1f counts/rev.\n",
           ANGLE_SETPOINTS_DEG[0],
           ENCODER_COUNTS_PER_REVOLUTION);
    printf("If the angle moves away from target, change ENCODER_DIRECTION to -1.\n");

    int64_t previous_time_us = esp_timer_get_time();
    int64_t previous_log_us = previous_time_us;
    int64_t setpoint_time_us = previous_time_us;
    size_t setpoint_index = 0;
    float setpoint_deg = ANGLE_SETPOINTS_DEG[setpoint_index];

    while (1) {
        int32_t encoder_count = 0;
        ESP_ERROR_CHECK(encoder_read(&encoder, &encoder_count));

        const float angle_deg =
            ((encoder_count - initial_count) * 360.0f /
             ENCODER_COUNTS_PER_REVOLUTION) *
            ENCODER_DIRECTION;

        const int64_t current_time_us = esp_timer_get_time();
        const int64_t setpoint_interval_us =
            static_cast<int64_t>(SETPOINT_INTERVAL_MS) * 1000;
        while (current_time_us - setpoint_time_us >= setpoint_interval_us) {
            setpoint_time_us += setpoint_interval_us;
            setpoint_index = (setpoint_index + 1) % SETPOINT_COUNT;
            setpoint_deg = ANGLE_SETPOINTS_DEG[setpoint_index];
            ESP_ERROR_CHECK(pid_reset(&angle_pid));
            printf("New angle target: %.1f deg.\n", setpoint_deg);
        }

        const float dt_seconds =
            static_cast<float>(current_time_us - previous_time_us) / 1000000.0f;
        previous_time_us = current_time_us;

        float output_percent = 0.0f;
        ESP_ERROR_CHECK(pid_compute(&angle_pid,
                                    setpoint_deg,
                                    angle_deg,
                                    dt_seconds,
                                    &output_percent));

        if (fabsf(setpoint_deg - angle_deg) <= ANGLE_TOLERANCE_DEG) {
            output_percent = 0.0f;
            ESP_ERROR_CHECK(pid_reset(&angle_pid));
        }

        ESP_ERROR_CHECK(drv8833_set_speed(
            &motor, static_cast<int16_t>(lroundf(output_percent))));

        if (current_time_us - previous_log_us >=
            static_cast<int64_t>(LOG_PERIOD_MS) * 1000) {
            printf("Angle: %.1f deg, target: %.1f deg, PWM: %.0f%%\n",
                   angle_deg,
                   setpoint_deg,
                   output_percent);
            previous_log_us = current_time_us;
        }

        vTaskDelay(pdMS_TO_TICKS(CONTROL_PERIOD_MS));
    }
}
