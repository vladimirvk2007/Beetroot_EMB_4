#include "sound/sound.h"

#define SOUND_SAMPLE_COUNT 16

static PwmDriver_t *sound_pwm = NULL;
static uint32_t sound_sample_index = 0;

static const uint32_t sound_sine_duty_percent[SOUND_SAMPLE_COUNT] = {
    50, 69, 85, 96, 100, 96, 85, 69,
    50, 31, 15, 4, 0, 4, 15, 31
};

uint32_t Sound_GetPwmFrequency(uint32_t frequency_hz) {
    return frequency_hz * SOUND_SAMPLE_COUNT;
}

static void Sound_UpdateDuty(void) {
    Pwm_SetDutyPercent(sound_pwm, sound_sine_duty_percent[sound_sample_index]);
    sound_sample_index++;
    if (sound_sample_index >= SOUND_SAMPLE_COUNT) {
        sound_sample_index = 0;
    }
}

static void Sound_HandleTimerInterrupt(TIM_TypeDef *timer_instance) {
    if (sound_pwm != NULL && sound_pwm->htim.Instance == timer_instance) {
        HAL_TIM_IRQHandler(&sound_pwm->htim);
    }
}

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (sound_pwm != NULL && &sound_pwm->htim == htim) {
        Sound_UpdateDuty();
    }
}

extern "C" void TIM2_IRQHandler(void) {
    Sound_HandleTimerInterrupt(TIM2);
}

extern "C" void TIM3_IRQHandler(void) {
    Sound_HandleTimerInterrupt(TIM3);
}

extern "C" void TIM4_IRQHandler(void) {
    Sound_HandleTimerInterrupt(TIM4);
}

extern "C" void TIM5_IRQHandler(void) {
    Sound_HandleTimerInterrupt(TIM5);
}

bool Sound_Init(PwmDriver_t *pwm, uint32_t frequency_hz) {
    if (pwm == NULL || !pwm->initialized || !pwm->running || frequency_hz == 0) {
        return false;
    }

    if (pwm->htim.Instance != TIM2 && pwm->htim.Instance != TIM3 &&
        pwm->htim.Instance != TIM4 && pwm->htim.Instance != TIM5) {
        return false;
    }

    Pwm_SetFrequency(pwm, Sound_GetPwmFrequency(frequency_hz));
    sound_pwm = pwm;
    sound_sample_index = 0;

    __HAL_TIM_CLEAR_FLAG(&sound_pwm->htim, TIM_FLAG_UPDATE);
    __HAL_TIM_ENABLE_IT(&sound_pwm->htim, TIM_IT_UPDATE);

    if (sound_pwm->htim.Instance == TIM2) {
        HAL_NVIC_SetPriority(TIM2_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(TIM2_IRQn);
    } else if (sound_pwm->htim.Instance == TIM3) {
        HAL_NVIC_SetPriority(TIM3_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(TIM3_IRQn);
    } else if (sound_pwm->htim.Instance == TIM4) {
        HAL_NVIC_SetPriority(TIM4_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(TIM4_IRQn);
    } else {
        HAL_NVIC_SetPriority(TIM5_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(TIM5_IRQn);
    }

    return true;
}

void Sound_Stop(void) {
    if (sound_pwm == NULL) {
        return;
    }

    __HAL_TIM_DISABLE_IT(&sound_pwm->htim, TIM_IT_UPDATE);
    sound_pwm = NULL;
}
