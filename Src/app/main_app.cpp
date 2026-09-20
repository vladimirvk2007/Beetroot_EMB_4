#include <stdio.h>
#include <stdbool.h>
#include "main.h"
#include "printf/usb_printf.h"
#include "pwm/pwm.h"
#include "servo/servo.h"

#define SERVO_GPIO_PORT PWM_PORT_B
#define SERVO_GPIO_PIN 4
#define SERVO_STEP_ANGLE 10

extern "C" void main_cpp() {
    bool error = false;

    PwmDriver_t servo_pwm;
    Servo_t servo;

    if (!Pwm_InitByPin(&servo_pwm, SERVO_GPIO_PORT,
                SERVO_GPIO_PIN, SERVO_FREQUENCY_HZ, 0)) {
        error = true;
        printf("PWM init failed\n");
    }

    if (!error && !Servo_Init(&servo, &servo_pwm)) {
        error = true;
        printf("Servo init failed\n");
    }

    while (1) {
        if (error) {
            printf("Init error\n");
        } else {
            for (uint16_t angle = SERVO_MIN_ANGLE;
                 angle <= SERVO_MAX_ANGLE; angle += SERVO_STEP_ANGLE) {
                Servo_SetAngle(&servo, angle);
                HAL_Delay(100);
            }

            for (int angle = SERVO_MAX_ANGLE; angle >= SERVO_MIN_ANGLE;
                 angle -= SERVO_STEP_ANGLE) {
                Servo_SetAngle(&servo, (uint16_t)angle);
                HAL_Delay(100);
            }
        }

        HAL_Delay(500);
    }
}
