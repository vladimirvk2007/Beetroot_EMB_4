#include "sine/sine.h"

#define SINE_SAMPLE_COUNT 16

static PwmDriver_t *sine_pwm = NULL;
static uint32_t sine_sample_index = 0;

static const uint32_t sine_sine_duty_percent[SINE_SAMPLE_COUNT] = {
    50, 69, 85, 96, 100, 96, 85, 69,
    50, 31, 15, 4, 0, 4, 15, 31
};

uint32_t Sine_GetPwmFrequency(uint32_t frequency_hz) {
    return frequency_hz * SINE_SAMPLE_COUNT;
}

static void Sine_UpdateDuty(void) {
    Pwm_SetDutyPercent(sine_pwm, sine_sine_duty_percent[sine_sample_index]);
    sine_sample_index++;
    if (sine_sample_index >= SINE_SAMPLE_COUNT) {
        sine_sample_index = 0;
    }
}

static void Sine_HandleTimerInterrupt(TIM_TypeDef *timer_instance) {
    if (sine_pwm != NULL && sine_pwm->htim.Instance == timer_instance) {
        HAL_TIM_IRQHandler(&sine_pwm->htim);
    }
}

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (sine_pwm != NULL && &sine_pwm->htim == htim) {
        Sine_UpdateDuty();
    }
}

extern "C" void TIM2_IRQHandler(void) {
    Sine_HandleTimerInterrupt(TIM2);
}

extern "C" void TIM3_IRQHandler(void) {
    Sine_HandleTimerInterrupt(TIM3);
}

extern "C" void TIM4_IRQHandler(void) {
    Sine_HandleTimerInterrupt(TIM4);
}

extern "C" void TIM5_IRQHandler(void) {
    Sine_HandleTimerInterrupt(TIM5);
}

bool Sine_Init(PwmDriver_t *pwm, uint32_t frequency_hz) {
    if (pwm == NULL || !pwm->initialized || !pwm->running || frequency_hz == 0) {
        return false;
    }

    if (pwm->htim.Instance != TIM2 && pwm->htim.Instance != TIM3 &&
        pwm->htim.Instance != TIM4 && pwm->htim.Instance != TIM5) {
        return false;
    }

    Pwm_SetFrequency(pwm, Sine_GetPwmFrequency(frequency_hz));
    sine_pwm = pwm;
    sine_sample_index = 0;

    __HAL_TIM_CLEAR_FLAG(&sine_pwm->htim, TIM_FLAG_UPDATE);
    __HAL_TIM_ENABLE_IT(&sine_pwm->htim, TIM_IT_UPDATE);

    if (sine_pwm->htim.Instance == TIM2) {
        HAL_NVIC_SetPriority(TIM2_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(TIM2_IRQn);
    } else if (sine_pwm->htim.Instance == TIM3) {
        HAL_NVIC_SetPriority(TIM3_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(TIM3_IRQn);
    } else if (sine_pwm->htim.Instance == TIM4) {
        HAL_NVIC_SetPriority(TIM4_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(TIM4_IRQn);
    } else {
        HAL_NVIC_SetPriority(TIM5_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(TIM5_IRQn);
    }

    return true;
}

void Sine_Stop(void) {
    if (sine_pwm == NULL) {
        return;
    }

    __HAL_TIM_DISABLE_IT(&sine_pwm->htim, TIM_IT_UPDATE);
    sine_pwm = NULL;
}
