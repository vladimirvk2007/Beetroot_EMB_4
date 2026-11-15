#ifndef ENCODER_H
#define ENCODER_H

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "esp_err.h"

// Контекст енкодера: GPIO каналів, кнопки та ресурси лічильника імпульсів.
typedef struct encoder_ctx_t {
    gpio_num_t a_gpio;
    gpio_num_t b_gpio;
    gpio_num_t button_gpio;
    uint32_t debounce_ns;
    pcnt_unit_handle_t pcnt_unit;
    pcnt_channel_handle_t chan_a;
    pcnt_channel_handle_t chan_b;
} encoder_ctx_t;

#ifdef __cplusplus
extern "C" {
#endif

// Ініціалізує квадратурний енкодер і, за потреби, кнопку.
// Для невикористаної кнопки передайте GPIO_NUM_NC.
esp_err_t encoder_init(encoder_ctx_t *ctx,
                       gpio_num_t a_gpio,
                       gpio_num_t b_gpio,
                       gpio_num_t button_gpio,
                       uint32_t debounce_ns);

// Зупиняє лічильник імпульсів і звільняє ресурси енкодера.
esp_err_t encoder_deinit(encoder_ctx_t *ctx);

// Зчитує поточне знакове значення лічильника імпульсів.
esp_err_t encoder_read(const encoder_ctx_t *ctx, int32_t *value);

// Повертає кількість імпульсів енкодера.
esp_err_t encoder_get_pulses(const encoder_ctx_t *ctx, int32_t *pulses);

// Перевіряє кнопку: true означає, що кнопка натиснута.
esp_err_t encoder_get_button(const encoder_ctx_t *ctx, bool *pressed);

#ifdef __cplusplus
}
#endif

#endif
