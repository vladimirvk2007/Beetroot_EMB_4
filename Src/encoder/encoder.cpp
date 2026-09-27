#include "encoder/encoder.h"

/* KY-040 only produces one active instance at a time in this project (see sine.cpp pattern). */
static EncoderCtx_t *encoder_active = NULL;

static void Encoder_ClockEnable(GPIO_TypeDef *gpio_port) {
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

static IRQn_Type Encoder_GetExtiIrqn(uint16_t pin) {
    switch (pin) {
        case GPIO_PIN_0: return EXTI0_IRQn;
        case GPIO_PIN_1: return EXTI1_IRQn;
        case GPIO_PIN_2: return EXTI2_IRQn;
        case GPIO_PIN_3: return EXTI3_IRQn;
        case GPIO_PIN_4: return EXTI4_IRQn;
        case GPIO_PIN_5:
        case GPIO_PIN_6:
        case GPIO_PIN_7:
        case GPIO_PIN_8:
        case GPIO_PIN_9:
            return EXTI9_5_IRQn;
        default:
            return EXTI15_10_IRQn;
    }
}

/* Gray-code transition table indexed by (old_state << 2 | new_state); 0 marks an
 * impossible (bounce) transition, so contact bounce cancels out instead of miscounting. */
static const int8_t kQuadTable[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0,
};

static void Encoder_HandleQuadratureEdge(EncoderCtx_t *ctx) {
    uint8_t a = (HAL_GPIO_ReadPin(ctx->a_port, ctx->a_pin) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t b = (HAL_GPIO_ReadPin(ctx->b_port, ctx->b_pin) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t new_state = (uint8_t)((a << 1) | b);
    uint8_t index = (uint8_t)((ctx->quad_state << 2) | new_state);
    ctx->quad_state = new_state;

    int8_t delta = kQuadTable[index];
    if (delta == 0) {
        return;
    }

    ctx->quad_accum = (int8_t)(ctx->quad_accum + delta);
    if (ctx->quad_accum >= 4) {
        ctx->pulses++;
        ctx->quad_accum = 0;
    } else if (ctx->quad_accum <= -4) {
        ctx->pulses--;
        ctx->quad_accum = 0;
    }
}

static void Encoder_HandleButtonEdge(EncoderCtx_t *ctx) {
    uint32_t now = HAL_GetTick();
    if (ctx->last_button_tick != 0 && ((now - ctx->last_button_tick) * 1000000UL) < ctx->debounce_ns) {
        return;
    }
    ctx->last_button_tick = now;
    ctx->button_pressed = (HAL_GPIO_ReadPin(ctx->button_port, ctx->button_pin) == GPIO_PIN_RESET);
}

extern "C" void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (encoder_active == NULL) {
        return;
    }

    if (GPIO_Pin == encoder_active->a_pin || GPIO_Pin == encoder_active->b_pin) {
        Encoder_HandleQuadratureEdge(encoder_active);
    } else if (GPIO_Pin == encoder_active->button_pin) {
        Encoder_HandleButtonEdge(encoder_active);
    }
}

extern "C" void EXTI0_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

extern "C" void EXTI1_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
}

extern "C" void EXTI2_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
}

extern "C" void EXTI3_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
}

extern "C" void EXTI4_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}

extern "C" void EXTI9_5_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_9);
}

extern "C" void EXTI15_10_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_14);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);
}

bool Encoder_Init(EncoderCtx_t *ctx,
                  GPIO_TypeDef *a_port, uint16_t a_pin,
                  GPIO_TypeDef *b_port, uint16_t b_pin,
                  GPIO_TypeDef *button_port, uint16_t button_pin,
                  uint32_t debounce_ns) {
    if (ctx == NULL || a_port == NULL || b_port == NULL || button_port == NULL) {
        return false;
    }

    if (ctx->initialized) {
        Encoder_Deinit(ctx);
    }

    ctx->a_port = a_port;
    ctx->a_pin = a_pin;
    ctx->b_port = b_port;
    ctx->b_pin = b_pin;
    ctx->button_port = button_port;
    ctx->button_pin = button_pin;
    ctx->debounce_ns = debounce_ns;
    ctx->pulses = 0;
    ctx->last_button_tick = 0;
    ctx->quad_accum = 0;
    ctx->initialized = false;

    Encoder_ClockEnable(a_port);
    Encoder_ClockEnable(b_port);
    Encoder_ClockEnable(button_port);

    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin = a_pin;
    gpio_init.Mode = GPIO_MODE_IT_RISING_FALLING;
    gpio_init.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(a_port, &gpio_init);

    gpio_init.Pin = b_pin;
    gpio_init.Mode = GPIO_MODE_IT_RISING_FALLING;
    gpio_init.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(b_port, &gpio_init);

    gpio_init.Pin = button_pin;
    gpio_init.Mode = GPIO_MODE_IT_RISING_FALLING;
    gpio_init.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(button_port, &gpio_init);

    ctx->button_pressed = (HAL_GPIO_ReadPin(button_port, button_pin) == GPIO_PIN_RESET);

    uint8_t a_level = (HAL_GPIO_ReadPin(a_port, a_pin) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t b_level = (HAL_GPIO_ReadPin(b_port, b_pin) == GPIO_PIN_SET) ? 1U : 0U;
    ctx->quad_state = (uint8_t)((a_level << 1) | b_level);

    encoder_active = ctx;
    ctx->initialized = true;

    IRQn_Type a_irq = Encoder_GetExtiIrqn(a_pin);
    HAL_NVIC_SetPriority(a_irq, 5, 0);
    HAL_NVIC_EnableIRQ(a_irq);

    IRQn_Type b_irq = Encoder_GetExtiIrqn(b_pin);
    if (b_irq != a_irq) {
        HAL_NVIC_SetPriority(b_irq, 5, 0);
        HAL_NVIC_EnableIRQ(b_irq);
    }

    IRQn_Type button_irq = Encoder_GetExtiIrqn(button_pin);
    if (button_irq != a_irq && button_irq != b_irq) {
        HAL_NVIC_SetPriority(button_irq, 5, 0);
        HAL_NVIC_EnableIRQ(button_irq);
    }

    return true;
}

void Encoder_Deinit(EncoderCtx_t *ctx) {
    if (ctx == NULL || !ctx->initialized) {
        return;
    }

    IRQn_Type a_irq = Encoder_GetExtiIrqn(ctx->a_pin);
    IRQn_Type b_irq = Encoder_GetExtiIrqn(ctx->b_pin);
    IRQn_Type button_irq = Encoder_GetExtiIrqn(ctx->button_pin);

    HAL_NVIC_DisableIRQ(a_irq);
    if (b_irq != a_irq) {
        HAL_NVIC_DisableIRQ(b_irq);
    }
    if (button_irq != a_irq && button_irq != b_irq) {
        HAL_NVIC_DisableIRQ(button_irq);
    }

    HAL_GPIO_DeInit(ctx->a_port, ctx->a_pin);
    HAL_GPIO_DeInit(ctx->b_port, ctx->b_pin);
    HAL_GPIO_DeInit(ctx->button_port, ctx->button_pin);

    if (encoder_active == ctx) {
        encoder_active = NULL;
    }

    ctx->initialized = false;
}

bool Encoder_Read(const EncoderCtx_t *ctx, int32_t *value) {
    if (ctx == NULL || value == NULL || !ctx->initialized) {
        return false;
    }

    *value = ctx->pulses;
    return true;
}

bool Encoder_GetPulses(const EncoderCtx_t *ctx, int32_t *pulses) {
    if (ctx == NULL || pulses == NULL || !ctx->initialized) {
        return false;
    }

    *pulses = ctx->pulses;
    return true;
}

bool Encoder_GetButton(const EncoderCtx_t *ctx, bool *pressed) {
    if (ctx == NULL || pressed == NULL || !ctx->initialized) {
        return false;
    }

    *pressed = ctx->button_pressed;
    return true;
}
