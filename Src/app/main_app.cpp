#include <stdio.h>
#include <stdbool.h>
#include "main.h"
#include "printf/usb_printf.h"
#include "pwm/pwm.h"

extern "C" void main_cpp() {
    PwmDriver_t pwm_led;
    PwmConfig_t pwm_cfg = {
        .port = PWM_PORT_A,
        .pin = 5,
        .af = GPIO_AF1_TIM2,
        .timer = PWM_TIM2,
        .channel = PWM_CH1,
        .frequency_hz = 1000,
        .duty_percent = 0
    };

    if (!Pwm_Init(&pwm_led, &pwm_cfg)) {
        printf("PWM init failed\n");
    }

    int32_t duty = 0;
    int32_t step = 10;

    while (1) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        HAL_Delay(50);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        HAL_Delay(50);

        Pwm_SetDutyPercent(&pwm_led, (uint32_t)duty);
        printf("PWM duty = %ld%%\n", (long)duty);

        duty += step;
        if (duty >= 100 || duty <= 0) {
            step = -step;
            duty += step;
        }

        HAL_Delay(100);
    }
}
