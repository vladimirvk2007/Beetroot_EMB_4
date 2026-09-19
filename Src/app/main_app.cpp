#include <stdio.h>
#include <stdbool.h>
#include "main.h"
#include "printf/usb_printf.h"
#include "pwm/pwm.h"
#include "sine/sine.h"

#define SINE_FREQUENCY_HZ 2200

extern "C" void main_cpp() {
    bool error = false;

    PwmDriver_t pwm_led;
    uint32_t pwm_frequency_hz = Sine_GetPwmFrequency(SINE_FREQUENCY_HZ);

    if (!Pwm_InitByPin(&pwm_led, PWM_PORT_B, 4, pwm_frequency_hz, 50)) {
        error = true;
        printf("PWM init failed\n");
    }

    if (!Sine_Init(&pwm_led, SINE_FREQUENCY_HZ)) {
        error = true;
        printf("Sine init failed\n");
    }

    while (1) {
        if (error) {
            printf("Init error\n");
        } else {
            printf("Running...\n");
        }

        HAL_Delay(1000);
    }
}
