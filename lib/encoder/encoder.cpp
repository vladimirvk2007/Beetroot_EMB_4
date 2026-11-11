#include "encoder.h"

#include <string.h>

#include "esp_log.h"

#define ENCODER_MIN_COUNT (-32768)
#define ENCODER_MAX_COUNT (32767)

static const char *TAG = "encoder";

esp_err_t encoder_init(encoder_ctx_t *ctx,
                      gpio_num_t a_gpio,
                      gpio_num_t b_gpio,
                      gpio_num_t button_gpio,
                      uint32_t debounce_ns) {
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->a_gpio = a_gpio;
    ctx->b_gpio = b_gpio;
    ctx->button_gpio = button_gpio;
    ctx->debounce_ns = debounce_ns;

    if (ctx->a_gpio == GPIO_NUM_NC || ctx->b_gpio == GPIO_NUM_NC) {
        return ESP_ERR_INVALID_STATE;
    }

    if (ctx->pcnt_unit != NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    pcnt_unit_config_t unit_config = {
        .low_limit = ENCODER_MIN_COUNT,
        .high_limit = ENCODER_MAX_COUNT,
        .intr_priority = 0,
        .flags = {
            .accum_count = 0,
        },
    };

    esp_err_t err = pcnt_new_unit(&unit_config, &ctx->pcnt_unit);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "pcnt_new_unit failed: %s", esp_err_to_name(err));
        return err;
    }

    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = ctx->debounce_ns,
    };
    err = pcnt_unit_set_glitch_filter(ctx->pcnt_unit, &filter_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "pcnt_unit_set_glitch_filter failed: %s", esp_err_to_name(err));
        return err;
    }

    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = ctx->a_gpio,
        .level_gpio_num = ctx->b_gpio,
        .flags = {
            .invert_edge_input = 0,
            .invert_level_input = 0,
            .virt_edge_io_level = 0,
            .virt_level_io_level = 0,
            .io_loop_back = 0,
        },
    };
    err = pcnt_new_channel(ctx->pcnt_unit, &chan_a_config, &ctx->chan_a);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "pcnt_new_channel(A) failed: %s", esp_err_to_name(err));
        return err;
    }

    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = ctx->b_gpio,
        .level_gpio_num = ctx->a_gpio,
        .flags = {
            .invert_edge_input = 0,
            .invert_level_input = 0,
            .virt_edge_io_level = 0,
            .virt_level_io_level = 0,
            .io_loop_back = 0,
        },
    };
    err = pcnt_new_channel(ctx->pcnt_unit, &chan_b_config, &ctx->chan_b);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "pcnt_new_channel(B) failed: %s", esp_err_to_name(err));
        return err;
    }

    err = pcnt_channel_set_edge_action(ctx->chan_a,
                                      PCNT_CHANNEL_EDGE_ACTION_DECREASE,
                                      PCNT_CHANNEL_EDGE_ACTION_INCREASE);
    if (err != ESP_OK) {
        return err;
    }
    err = pcnt_channel_set_level_action(ctx->chan_a,
                                       PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                       PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    if (err != ESP_OK) {
        return err;
    }

    err = pcnt_channel_set_edge_action(ctx->chan_b,
                                      PCNT_CHANNEL_EDGE_ACTION_INCREASE,
                                      PCNT_CHANNEL_EDGE_ACTION_DECREASE);
    if (err != ESP_OK) {
        return err;
    }
    err = pcnt_channel_set_level_action(ctx->chan_b,
                                       PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                       PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    if (err != ESP_OK) {
        return err;
    }

    err = pcnt_unit_enable(ctx->pcnt_unit);
    if (err != ESP_OK) {
        pcnt_del_unit(ctx->pcnt_unit);
        ctx->pcnt_unit = NULL;
        return err;
    }

    err = pcnt_unit_clear_count(ctx->pcnt_unit);
    if (err != ESP_OK) {
        pcnt_unit_disable(ctx->pcnt_unit);
        pcnt_del_unit(ctx->pcnt_unit);
        ctx->pcnt_unit = NULL;
        return err;
    }

    err = pcnt_unit_start(ctx->pcnt_unit);
    if (err != ESP_OK) {
        pcnt_unit_disable(ctx->pcnt_unit);
        pcnt_del_unit(ctx->pcnt_unit);
        ctx->pcnt_unit = NULL;
        return err;
    }

    if (ctx->button_gpio != GPIO_NUM_NC) {
        gpio_set_direction(ctx->button_gpio, GPIO_MODE_INPUT);
        gpio_set_pull_mode(ctx->button_gpio, GPIO_PULLUP_ONLY);
    }

    return ESP_OK;
}

esp_err_t encoder_deinit(encoder_ctx_t *ctx) {
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (ctx->pcnt_unit != NULL) {
        esp_err_t err = pcnt_unit_disable(ctx->pcnt_unit);
        if (err != ESP_OK) {
            return err;
        }

        err = pcnt_del_unit(ctx->pcnt_unit);
        if (err != ESP_OK) {
            return err;
        }

        ctx->pcnt_unit = NULL;
    }

    ctx->chan_a = NULL;
    ctx->chan_b = NULL;
    return ESP_OK;
}

esp_err_t encoder_read(const encoder_ctx_t *ctx, int32_t *value) {
    if (ctx == NULL || value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (ctx->pcnt_unit == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    int raw_count = 0;
    esp_err_t err = pcnt_unit_get_count(ctx->pcnt_unit, &raw_count);
    if (err != ESP_OK) {
        return err;
    }

    *value = (int32_t)raw_count;
    return ESP_OK;
}

esp_err_t encoder_get_pulses(const encoder_ctx_t *ctx, int32_t *pulses) {
    if (ctx == NULL || pulses == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (ctx->pcnt_unit == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    return encoder_read(ctx, pulses);
}

esp_err_t encoder_get_button(const encoder_ctx_t *ctx, bool *pressed) {
    if (ctx == NULL || pressed == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (ctx->button_gpio == GPIO_NUM_NC) {
        *pressed = false;
        return ESP_OK;
    }

    *pressed = gpio_get_level(ctx->button_gpio) == 0;
    return ESP_OK;
}

