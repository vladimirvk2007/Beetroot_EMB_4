#ifndef ENCODER_H
#define ENCODER_H

#include "main.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    GPIO_TypeDef *a_port;
    uint16_t a_pin;
    GPIO_TypeDef *b_port;
    uint16_t b_pin;
    GPIO_TypeDef *button_port;
    uint16_t button_pin;
    uint32_t debounce_ns;
    volatile int32_t pulses;
    volatile bool button_pressed;
    volatile uint32_t last_button_tick;
    volatile uint8_t quad_state;
    volatile int8_t quad_accum;
    bool initialized;
} EncoderCtx_t;

/*
 * a_port/a_pin, b_port/b_pin і button_port/button_pin - будь-який GPIOx (A..E) і GPIO_PIN_0..15,
 * але з різними номерами пінів (кожен займає свою лінію EXTI0..15), бо всі три лінії
 * використовують переривання для стійкого до дребезгу декодування квадратури.
 * Приклад: Encoder_Init(&ctx, GPIOA, GPIO_PIN_0, GPIOA, GPIO_PIN_1,
 *                        GPIOB, GPIO_PIN_2, 2000000);
 * Заборонено (a_pin і button_pin мають однаковий номер GPIO_PIN_0,
 * навіть на різних портах GPIOA/GPIOB - конфлікт на лінії EXTI0):
 * Encoder_Init(&ctx, GPIOA, GPIO_PIN_0, GPIOA, GPIO_PIN_1,
 *              GPIOB, GPIO_PIN_0, 2000000);
 */
bool Encoder_Init(EncoderCtx_t *ctx,
                  GPIO_TypeDef *a_port, uint16_t a_pin,
                  GPIO_TypeDef *b_port, uint16_t b_pin,
                  GPIO_TypeDef *button_port, uint16_t button_pin,
                  uint32_t debounce_ns);
void Encoder_Deinit(EncoderCtx_t *ctx);
bool Encoder_Read(const EncoderCtx_t *ctx, int32_t *value);
bool Encoder_GetPulses(const EncoderCtx_t *ctx, int32_t *pulses);
bool Encoder_GetButton(const EncoderCtx_t *ctx, bool *pressed);

#endif
