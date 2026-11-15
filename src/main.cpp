#include <math.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "etl/atomic.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "drv8833.h"
#include "encoder.h"
#include "pid.h"

using namespace etl;

// Драйвер DRV8833.
#define MOTOR_IN1 GPIO_NUM_16
#define MOTOR_IN2 GPIO_NUM_17

// Канали KY-040 та калібрування енкодера.
#define ENCODER_A GPIO_NUM_7
#define ENCODER_B GPIO_NUM_15
#define ENCODER_COUNTS_PER_REVOLUTION 80
#define ENCODER_DIRECTION 1
#define ENCODER_GLITCH_FILTER_NS 10000

// Налаштування PWM, точність позиціювання та інтервали керування.
#define PWM_FREQUENCY_HZ 2000
#define PWM_RESOLUTION LEDC_TIMER_12_BIT
#define ANGLE_TOLERANCE_DEG 5
#define MIN_MOVING_PWM_PERCENT 50
#define CONTROL_PERIOD_MS 5
#define LOG_PERIOD_MS 300
#define SETPOINT_INTERVAL_MS 3000

// PID-обчислення виконуються таймером кожні CONTROL_PERIOD_MS мілісекунд.
// Нова цільова позиція вибирається кожні SETPOINT_INTERVAL_MS мілісекунд.

// Послідовність цільових кутів відносно положення енкодера під час запуску.
static const float ANGLE_SETPOINTS_DEG[] = {
    0.0f, 45.0f, 90.0f, 135.0f, 180.0f, 215.0f, 260.0f, 305.0f
};
#define SETPOINT_COUNT (sizeof(ANGLE_SETPOINTS_DEG) / sizeof(ANGLE_SETPOINTS_DEG[0]))

// Додатний вихід PID задає напрямок IN1, від'ємний — напрямок IN2.
// Коефіцієнти PID; обмеження виходу задаються у відсотках PWM.
static const pid_config_t PID_CONFIG = {
    .kp = 5.0f,
    .ki = 7.0f,
    .kd = 30.0f,
    .output_min = -100.0f,
    .output_max = 100.0f,
    .integral_min = -500.0f,
    .integral_max = 500.0f,
};

// Ненульову команду PID масштабує до робочого діапазону PWM,
// зберігаючи знак, який визначає напрямок обертання.
static float map_motor_pwm(float output_percent) {
    if (output_percent == 0.0f) {
        return 0.0f;
    }

    const float magnitude = MIN_MOVING_PWM_PERCENT +
        fabsf(output_percent) * (100.0f - MIN_MOVING_PWM_PERCENT) / 100.0f;
    return copysignf(magnitude, output_percent);
}

struct motor_control_state_t {
    drv8833_t *motor;
    encoder_ctx_t *encoder;
    pid_controller_t *pid;
    int32_t initial_count;
    int64_t setpoint_time_us;
    size_t setpoint_index;
    float setpoint_deg;
};

// Атомарні значення для обміну даними між callback таймера та циклом логування.
static atomic<int32_t> log_encoder_count{0};
static atomic<size_t> log_setpoint_index{0};
static atomic<int8_t> log_output_percent{0};

// Періодично зчитує енкодер, оновлює уставку, виконує PID і керує двигуном.
static void control_timer_callback(void *arg) {
    motor_control_state_t *state = static_cast<motor_control_state_t *>(arg);

    int32_t encoder_count = 0;
    ESP_ERROR_CHECK(encoder_read(state->encoder, &encoder_count));

    // Перетворює зміну відліків від стартового положення на кут у градусах.
    const float angle_deg =
        ((encoder_count - state->initial_count) * 360.0f /
         ENCODER_COUNTS_PER_REVOLUTION) *
        ENCODER_DIRECTION;

    const int64_t current_time_us = esp_timer_get_time();
    const int64_t setpoint_interval_us =
        static_cast<int64_t>(SETPOINT_INTERVAL_MS) * 1000;
    // Перемикає уставку за розкладом і скидає історію попередньої цілі PID.
    while (current_time_us - state->setpoint_time_us >= setpoint_interval_us) {
        state->setpoint_time_us += setpoint_interval_us;
        state->setpoint_index = (state->setpoint_index + 1) % SETPOINT_COUNT;
        state->setpoint_deg = ANGLE_SETPOINTS_DEG[state->setpoint_index];
        ESP_ERROR_CHECK(pid_reset(state->pid));
    }

    int8_t pid_output_percent = 0;
    ESP_ERROR_CHECK(pid_compute(state->pid,
                                state->setpoint_deg,
                                angle_deg,
                                CONTROL_PERIOD_MS,
                                &pid_output_percent));

    // У зоні допуску зупиняє двигун і прибирає накопичений інтеграл.
    if (fabsf(state->setpoint_deg - angle_deg) <= ANGLE_TOLERANCE_DEG) {
        pid_output_percent = 0;
        ESP_ERROR_CHECK(pid_reset(state->pid));
    }

    const float output_percent = map_motor_pwm(pid_output_percent);
    ESP_ERROR_CHECK(drv8833_set_speed(
        state->motor, static_cast<int16_t>(lroundf(output_percent))));

    // Зберігає останні дані для виведення в основному циклі.
    log_encoder_count.store(encoder_count, memory_order_relaxed);
    log_setpoint_index.store(state->setpoint_index, memory_order_relaxed);
    log_output_percent.store(
        static_cast<int8_t>(lroundf(output_percent)),
        memory_order_relaxed);
}

extern "C" void app_main() {
    drv8833_t motor = {};
    // Обидва PWM-канали використовують один таймер, частоту й роздільність.
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

    // Поточний відлік енкодера задає нульову позицію для кутів у програмі.
    int32_t initial_count = 0;
    ESP_ERROR_CHECK(encoder_read(&encoder, &initial_count));

    printf("Starting angle target: %.1f deg; encoder: %d counts/rev.\n",
           ANGLE_SETPOINTS_DEG[0],
           ENCODER_COUNTS_PER_REVOLUTION);
    printf("If the angle moves away from target, change ENCODER_DIRECTION to -1.\n");

    const int64_t start_time_us = esp_timer_get_time();
    motor_control_state_t control_state = {
        .motor = &motor,
        .encoder = &encoder,
        .pid = &angle_pid,
        .initial_count = initial_count,
        .setpoint_time_us = start_time_us,
        .setpoint_index = 0,
        .setpoint_deg = ANGLE_SETPOINTS_DEG[0],
    };

    // Таймер запускає цикл керування окремо від повільнішого логування.
    esp_timer_create_args_t timer_args = {};
    timer_args.callback = control_timer_callback;
    timer_args.arg = &control_state;
    timer_args.dispatch_method = ESP_TIMER_TASK;
    timer_args.name = "motor_control";
    esp_timer_handle_t control_timer = nullptr;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &control_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(
        control_timer, static_cast<uint64_t>(CONTROL_PERIOD_MS) * 1000ULL));

    float previous_logged_setpoint = ANGLE_SETPOINTS_DEG[0];
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(LOG_PERIOD_MS));

        // Виводить останні атомарно збережені показання, не затримуючи PID.
        const int32_t encoder_count =
            log_encoder_count.load(memory_order_relaxed);
        const size_t setpoint_index =
            log_setpoint_index.load(memory_order_relaxed);
        const int8_t output_percent =
            log_output_percent.load(memory_order_relaxed);
        const float angle_deg =
            ((static_cast<int64_t>(encoder_count) - initial_count) * 360.0f /
             ENCODER_COUNTS_PER_REVOLUTION) *
            ENCODER_DIRECTION;
        const float setpoint_deg = ANGLE_SETPOINTS_DEG[setpoint_index];

        if (setpoint_deg != previous_logged_setpoint) {
            printf("New angle target: %.1f deg.\n", setpoint_deg);
            previous_logged_setpoint = setpoint_deg;
        }

        printf("Angle: %.1f deg, target: %.1f deg, PWM: %.0f%%\n",
               angle_deg,
               setpoint_deg,
               static_cast<float>(output_percent));
    }
}
