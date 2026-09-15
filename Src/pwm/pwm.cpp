#include "pwm/pwm.h"

static GPIO_TypeDef *Pwm_GetPort(PwmPort_t port) {
    switch (port) {
        case PWM_PORT_A: return GPIOA;
        case PWM_PORT_B: return GPIOB;
        case PWM_PORT_C: return GPIOC;
        case PWM_PORT_D: return GPIOD;
        case PWM_PORT_E: return GPIOE;
        default: return NULL;
    }
}

static TIM_TypeDef *Pwm_GetTimer(PwmTimer_t timer) {
    switch (timer) {
        case PWM_TIM1: return TIM1;
        case PWM_TIM2: return TIM2;
        case PWM_TIM3: return TIM3;
        case PWM_TIM4: return TIM4;
        case PWM_TIM5: return TIM5;
        case PWM_TIM9: return TIM9;
        case PWM_TIM10: return TIM10;
        case PWM_TIM11: return TIM11;
        default: return NULL;
    }
}

static uint32_t Pwm_GetTimerChannel(PwmChannel_t channel) {
    switch (channel) {
        case PWM_CH1: return TIM_CHANNEL_1;
        case PWM_CH2: return TIM_CHANNEL_2;
        case PWM_CH3: return TIM_CHANNEL_3;
        case PWM_CH4: return TIM_CHANNEL_4;
        default: return 0U;
    }
}

static uint32_t Pwm_GetTimerClockHz(TIM_TypeDef *timer_inst) {
    if (timer_inst == TIM1 || timer_inst == TIM9 || timer_inst == TIM10 ||
        timer_inst == TIM11) {
        return HAL_RCC_GetPCLK2Freq();
    }
    return HAL_RCC_GetPCLK1Freq();
}

static void Pwm_ClockEnable(GPIO_TypeDef *gpio_port) {
    if (gpio_port == GPIOA) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    } else if (gpio_port == GPIOB) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    } else if (gpio_port == GPIOC) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    } else if (gpio_port == GPIOD) {
        __HAL_RCC_GPIOD_CLK_ENABLE();
    } else if (gpio_port == GPIOE) {
        __HAL_RCC_GPIOE_CLK_ENABLE();
    }
}

static void Pwm_TimerClockEnable(TIM_TypeDef *timer_inst) {
    if (timer_inst == TIM1) {
        __HAL_RCC_TIM1_CLK_ENABLE();
    } else if (timer_inst == TIM2) {
        __HAL_RCC_TIM2_CLK_ENABLE();
    } else if (timer_inst == TIM3) {
        __HAL_RCC_TIM3_CLK_ENABLE();
    } else if (timer_inst == TIM4) {
        __HAL_RCC_TIM4_CLK_ENABLE();
    } else if (timer_inst == TIM5) {
        __HAL_RCC_TIM5_CLK_ENABLE();
    } else if (timer_inst == TIM9) {
        __HAL_RCC_TIM9_CLK_ENABLE();
    } else if (timer_inst == TIM10) {
        __HAL_RCC_TIM10_CLK_ENABLE();
    } else if (timer_inst == TIM11) {
        __HAL_RCC_TIM11_CLK_ENABLE();
    }
}

static void Pwm_ConfigureTimer(PwmDriver_t *driver, TIM_TypeDef *timer_inst, uint32_t frequency_hz) {
    uint32_t timer_clock_hz = Pwm_GetTimerClockHz(timer_inst);
    uint32_t target_period = timer_clock_hz / frequency_hz;
    uint32_t prescaler = 0U;

    while (target_period > 65535U && prescaler < 65535U) {
        prescaler++;
        target_period = timer_clock_hz / ((prescaler + 1U) * frequency_hz);
    }

    if (target_period == 0U) {
        target_period = 1U;
    }

    driver->prescaler = prescaler;
    driver->period = target_period - 1U;

    driver->htim.Instance = timer_inst;
    driver->htim.Init.Prescaler = driver->prescaler;
    driver->htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    driver->htim.Init.Period = driver->period;
    driver->htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    driver->htim.Init.RepetitionCounter = 0U;
    driver->htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
}

bool Pwm_Init(PwmDriver_t *driver, const PwmConfig_t *config) {
    if (driver == NULL || config == NULL) {
        return false;
    }

    GPIO_TypeDef *gpio_port = Pwm_GetPort(config->port);
    TIM_TypeDef *timer_inst = Pwm_GetTimer(config->timer);
    if (gpio_port == NULL || timer_inst == NULL || config->frequency_hz == 0U) {
        return false;
    }

    Pwm_Deinit(driver);

    driver->gpio_port = gpio_port;
    driver->pin = config->pin;
    driver->channel = Pwm_GetTimerChannel(config->channel);
    driver->initialized = false;
    driver->running = false;

    Pwm_ClockEnable(gpio_port);
    Pwm_TimerClockEnable(timer_inst);

    GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin = (1U << config->pin);
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = config->af;
    HAL_GPIO_Init(gpio_port, &gpio_init);

    Pwm_ConfigureTimer(driver, timer_inst, config->frequency_hz);

    if (HAL_TIM_PWM_Init(&driver->htim) != HAL_OK) {
        return false;
    }

    TIM_OC_InitTypeDef pwm_config = {0};
    pwm_config.OCMode = TIM_OCMODE_PWM1;
    pwm_config.Pulse = 0U;
    pwm_config.OCPolarity = TIM_OCPOLARITY_HIGH;
    pwm_config.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(&driver->htim, &pwm_config, driver->channel) != HAL_OK) {
        return false;
    }

    driver->initialized = true;
    Pwm_SetDutyPercent(driver, config->duty_percent);
    Pwm_Start(driver);
    return true;
}

void Pwm_Deinit(PwmDriver_t *driver) {
    if (driver == NULL) {
        return;
    }

    Pwm_Stop(driver);
    driver->htim.Instance = NULL;
    driver->gpio_port = NULL;
    driver->pin = 0U;
    driver->channel = 0U;
    driver->prescaler = 0U;
    driver->period = 0U;
    driver->initialized = false;
    driver->running = false;
}

void Pwm_Start(PwmDriver_t *driver) {
    if (driver == NULL || !driver->initialized || driver->htim.Instance == NULL) {
        return;
    }

    if (HAL_TIM_PWM_Start(&driver->htim, driver->channel) == HAL_OK) {
        driver->running = true;
    }
}

void Pwm_Stop(PwmDriver_t *driver) {
    if (driver == NULL || driver->htim.Instance == NULL) {
        return;
    }

    HAL_TIM_PWM_Stop(&driver->htim, driver->channel);
    driver->running = false;

    if (driver->gpio_port != NULL && driver->pin <= 15U) {
        GPIO_InitTypeDef gpio_init = {0};
        gpio_init.Pin = (1U << driver->pin);
        gpio_init.Mode = GPIO_MODE_INPUT;
        gpio_init.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(driver->gpio_port, &gpio_init);
    }
}

void Pwm_SetDutyPercent(PwmDriver_t *driver, uint32_t duty_percent) {
    if (driver == NULL || !driver->initialized || driver->htim.Instance == NULL) {
        return;
    }

    if (duty_percent > 100U) {
        duty_percent = 100U;
    }

    uint32_t value = (driver->period * duty_percent) / 100U;
    __HAL_TIM_SET_COMPARE(&driver->htim, driver->channel, value);
}

void Pwm_SetDutyCycle(PwmDriver_t *driver, uint32_t compare_value) {
    if (driver == NULL || !driver->initialized || driver->htim.Instance == NULL) {
        return;
    }

    if (compare_value > driver->period) {
        compare_value = driver->period;
    }

    __HAL_TIM_SET_COMPARE(&driver->htim, driver->channel, compare_value);
}

void Pwm_SetFrequency(PwmDriver_t *driver, uint32_t frequency_hz) {
    if (driver == NULL || !driver->initialized || driver->htim.Instance == NULL || frequency_hz == 0U) {
        return;
    }

    Pwm_ConfigureTimer(driver, driver->htim.Instance, frequency_hz);

    if (HAL_TIM_PWM_Init(&driver->htim) != HAL_OK) {
        return;
    }

    TIM_OC_InitTypeDef pwm_config = {0};
    pwm_config.OCMode = TIM_OCMODE_PWM1;
    pwm_config.Pulse = driver->period / 2U;
    pwm_config.OCPolarity = TIM_OCPOLARITY_HIGH;
    pwm_config.OCFastMode = TIM_OCFAST_DISABLE;

    HAL_TIM_PWM_ConfigChannel(&driver->htim, &pwm_config, driver->channel);
    if (driver->running) {
        HAL_TIM_PWM_Start(&driver->htim, driver->channel);
    }
}

bool Pwm_IsRunning(const PwmDriver_t *driver) {
    return (driver != NULL) ? driver->running : false;
}
