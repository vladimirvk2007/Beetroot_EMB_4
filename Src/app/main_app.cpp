#include <stdio.h>
#include <stdbool.h>
#include "main.h"
#include "printf/usb_printf.h"
#include "pwm/pwm.h"
#include "sine/sine.h"
#include "encoder/encoder.h"

#define SINE_FREQUENCY_HZ 2200

#define ENCODER_A_PORT GPIOA
#define ENCODER_A_PIN GPIO_PIN_0
#define ENCODER_B_PORT GPIOA
#define ENCODER_B_PIN GPIO_PIN_1
#define ENCODER_BUTTON_PORT GPIOA
#define ENCODER_BUTTON_PIN GPIO_PIN_2
#define ENCODER_DEBOUNCE_NS 2000000

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

    EncoderCtx_t encoder = {0};
    if (!Encoder_Init(&encoder, ENCODER_A_PORT, ENCODER_A_PIN,
                      ENCODER_B_PORT, ENCODER_B_PIN,
                      ENCODER_BUTTON_PORT, ENCODER_BUTTON_PIN,
                      ENCODER_DEBOUNCE_NS)) {
        error = true;
        printf("Encoder init failed\n");
    }

    int32_t last_position = 0;
    bool last_pressed = false;
    GPIO_PinState last_a_state = HAL_GPIO_ReadPin(ENCODER_A_PORT, ENCODER_A_PIN);
    GPIO_PinState last_b_state = HAL_GPIO_ReadPin(ENCODER_B_PORT, ENCODER_B_PIN);

    while (1) {
        if (error) {
            printf("Init error\n");
        } else {
            int32_t position = 0;
            bool pressed = false;

            Encoder_Read(&encoder, &position);
            Encoder_GetButton(&encoder, &pressed);
            GPIO_PinState a_state = HAL_GPIO_ReadPin(ENCODER_A_PORT, ENCODER_A_PIN);
            GPIO_PinState b_state = HAL_GPIO_ReadPin(ENCODER_B_PORT, ENCODER_B_PIN);

            if (position != last_position ||
                pressed != last_pressed ||
                a_state != last_a_state ||
                b_state != last_b_state) {
                printf("Encoder position: %ld, button: %s, A: %d, B: %d\n",
                       (long)position, pressed ? "pressed" : "released",
                       (int)a_state, (int)b_state);
                last_position = position;
                last_pressed = pressed;
                last_a_state = a_state;
                last_b_state = b_state;
            }
        }

        HAL_Delay(10);
    }
}
