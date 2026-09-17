#include <stdio.h>
#include <stdbool.h>
#include "main.h"
#include "printf/usb_printf.h"
#include "pwm/pwm.h"
#include "sound/sound.h"

#define SOUND_FREQUENCY_HZ 2200

extern "C" void main_cpp() {
    PwmDriver_t pwm_led;
    uint32_t pwm_frequency_hz = Sound_GetPwmFrequency(SOUND_FREQUENCY_HZ);

    if (!Pwm_InitByPin(&pwm_led, PWM_PORT_B, 4, pwm_frequency_hz, 50)) {
        printf("PWM init failed\n");
    }

    if (!Sound_Init(&pwm_led, SOUND_FREQUENCY_HZ)) {
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
