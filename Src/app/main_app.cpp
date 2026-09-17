#include <stdio.h>
#include <stdbool.h>
#include "main.h"
#include "printf/usb_printf.h"
#include "pwm/pwm.h"
#include "sound/sound.h"

extern "C" void main_cpp() {
    PwmDriver_t pwm_led;

    if (!Pwm_InitByPin(&pwm_led, PWM_PORT_B, 4, 16000, 50)) {
        printf("PWM init failed\n");
    }

    if (!Sound_Init(&pwm_led, 1000)) {
        printf("Sound init failed\n");
    }

    while (1) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        HAL_Delay(50);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        HAL_Delay(50);

        HAL_Delay(100);
    }
}
