#ifndef ENCODER_H
#define ENCODER_H

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "esp_err.h"

typedef struct encoder_ctx_t {
    gpio_num_t a_gpio;
    gpio_num_t b_gpio;
    gpio_num_t button_gpio;
    uint32_t debounce_ns;
    pcnt_unit_handle_t pcnt_unit;
    pcnt_channel_handle_t chan_a;
    pcnt_channel_handle_t chan_b;
} encoder_ctx_t;

esp_err_t encoder_init(encoder_ctx_t *ctx,
                       gpio_num_t a_gpio,
                       gpio_num_t b_gpio,
                       gpio_num_t button_gpio,
                       uint32_t debounce_ns);

esp_err_t encoder_deinit(encoder_ctx_t *ctx);
esp_err_t encoder_read(const encoder_ctx_t *ctx, int32_t *value);
esp_err_t encoder_get_pulses(const encoder_ctx_t *ctx, int32_t *pulses);
esp_err_t encoder_get_button(const encoder_ctx_t *ctx, bool *pressed);

#endif
