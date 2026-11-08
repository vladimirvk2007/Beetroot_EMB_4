#include <stdio.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pwm.h"

// DRV8833 control pins
#define MOTOR_IN1 GPIO_NUM_16  // Forward
#define MOTOR_IN2 GPIO_NUM_17  // Reverse

// Timing constants (milliseconds)
#define FORWARD_TIME_MS 10000  // 10 seconds
#define BRAKE_TIME_MS 2000     // 2 seconds
#define REVERSE_TIME_MS 10000  // 10 seconds

// DRV8833 Control States
// IN1 IN2 | OUT1 OUT2 | Function
// 0   0   | Z    Z    | Coast (Fast Decay)
// 0   1   | L    H    | Reverse
// 1   0   | H    L    | Forward
// 1   1   | L    L    | Brake (Slow Decay)

void set_motor_state(uint8_t in1, uint8_t in2) {
    gpio_set_level(MOTOR_IN1, in1);
    gpio_set_level(MOTOR_IN2, in2);
}

extern "C" void app_main() {
    // Configure GPIO pins as outputs
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << MOTOR_IN1) | (1ULL << MOTOR_IN2),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Initial state: Coast
    set_motor_state(0, 0);

    printf("Motor control started!\n");
    printf("Cycle: Forward (10s) → Coast Brake (2s) → Reverse (10s) → Coast Brake (2s) → repeat\n");

    while (1) {
        // Forward (IN1=1, IN2=0)
        printf("[FORWARD] Motor rotating forward...\n");
        set_motor_state(1, 0);
        vTaskDelay(FORWARD_TIME_MS / portTICK_PERIOD_MS);

        // Coast Brake (IN1=0, IN2=0)
        printf("[COAST BRAKE] Smooth deceleration...\n");
        set_motor_state(0, 0);
        vTaskDelay(pdMS_TO_TICKS(BRAKE_TIME_MS));

        // Reverse (IN1=0, IN2=1)
        printf("[REVERSE] Motor rotating reverse...\n");
        set_motor_state(0, 1);
        vTaskDelay(REVERSE_TIME_MS / portTICK_PERIOD_MS);

        // Coast Brake (IN1=0, IN2=0)
        printf("[COAST BRAKE] Smooth deceleration...\n");
        set_motor_state(0, 0);
        vTaskDelay(pdMS_TO_TICKS(BRAKE_TIME_MS));
    }
}
