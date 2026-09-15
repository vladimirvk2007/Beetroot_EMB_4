#ifndef PWM_H
#define PWM_H

#include "main.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    PWM_PORT_A = 0,
    PWM_PORT_B,
    PWM_PORT_C,
    PWM_PORT_D,
    PWM_PORT_E,
} PwmPort_t;

typedef enum {
    PWM_TIM1 = 0,
    PWM_TIM2,
    PWM_TIM3,
    PWM_TIM4,
    PWM_TIM5,
    PWM_TIM9,
    PWM_TIM10,
    PWM_TIM11,
} PwmTimer_t;

typedef enum {
    PWM_CH1 = 1,
    PWM_CH2 = 2,
    PWM_CH3 = 3,
    PWM_CH4 = 4,
} PwmChannel_t;

typedef struct {
    PwmPort_t port;
    uint16_t pin;
    uint8_t af;
    PwmTimer_t timer;
    PwmChannel_t channel;
    uint32_t frequency_hz;
    uint32_t duty_percent;
} PwmConfig_t;

typedef struct {
    TIM_HandleTypeDef htim;
    GPIO_TypeDef *gpio_port;
    uint16_t pin;
    uint32_t channel;
    uint32_t prescaler;
    uint32_t period;
    bool initialized;
    bool running;
} PwmDriver_t;

bool Pwm_Init(PwmDriver_t *driver, const PwmConfig_t *config);
void Pwm_Deinit(PwmDriver_t *driver);
void Pwm_Start(PwmDriver_t *driver);
void Pwm_Stop(PwmDriver_t *driver);
void Pwm_SetDutyPercent(PwmDriver_t *driver, uint32_t duty_percent);
void Pwm_SetDutyCycle(PwmDriver_t *driver, uint32_t compare_value);
void Pwm_SetFrequency(PwmDriver_t *driver, uint32_t frequency_hz);
bool Pwm_IsRunning(const PwmDriver_t *driver);

#endif
