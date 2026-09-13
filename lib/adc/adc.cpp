#include <string.h>
#include "adc.h"

static const char *TAG = "ADC_DRIVER";

// 1. КАЛІБРУВАННЯ (CALIBRATION)
esp_err_t adc_drv_cali_init(adc_drv_cali_ctx_t *ctx,
                            adc_unit_t unit, adc_channel_t chan,
                            adc_atten_t atten,
                            adc_bitwidth_t bitwidth) {
    if (!ctx) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(ctx, 0, sizeof(adc_drv_cali_ctx_t));
    ctx->atten = atten;
    ctx->bitwidth = bitwidth;
    ctx->cali_handle = NULL;

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .chan = chan,
        .atten = atten,
        .bitwidth = bitwidth,
    };
    esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_config, &ctx->cali_handle);
    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "Calibration Curve Fitting created: Unit %d, Chan %d", unit, chan);
    } else {
        ESP_LOGE(TAG, "Failed to create calibration: %s", esp_err_to_name(ret));
    }
    return ret;
}

void adc_drv_cali_deinit(adc_drv_cali_ctx_t *ctx) {
    if (!ctx) return;

    if (ctx->cali_handle) {
        adc_cali_delete_scheme_curve_fitting(ctx->cali_handle);
        ctx->cali_handle = NULL;
    }
}

esp_err_t adc_drv_cali_raw_to_voltage(const adc_drv_cali_ctx_t *ctx, int raw_val, int *voltage_mv_out) {
    if (!ctx || !voltage_mv_out) {
        return ESP_ERR_INVALID_ARG;
    }

    if (ctx->cali_handle) {
        return adc_cali_raw_to_voltage(ctx->cali_handle, raw_val, voltage_mv_out);
    }

    *voltage_mv_out = adc_drv_cali_estimate_voltage(raw_val, ctx->atten, ctx->bitwidth);
    return ESP_OK;
}

int adc_drv_cali_estimate_voltage(int raw_val, adc_atten_t atten, adc_bitwidth_t bitwidth) {
    int max_raw = 4095;
    if (bitwidth == ADC_BITWIDTH_9) max_raw = 511;
    else if (bitwidth == ADC_BITWIDTH_10) max_raw = 1023;
    else if (bitwidth == ADC_BITWIDTH_11) max_raw = 2047;
    else if (bitwidth == ADC_BITWIDTH_12 || bitwidth == ADC_BITWIDTH_DEFAULT) max_raw = 4095;
    else if (bitwidth == ADC_BITWIDTH_13) max_raw = 8191;

    int max_mv = 1100;
    switch (atten) {
        case ADC_ATTEN_DB_0:   max_mv = 950;  break;
        case ADC_ATTEN_DB_2_5: max_mv = 1250; break;
        case ADC_ATTEN_DB_6:   max_mv = 1750; break;
        case ADC_ATTEN_DB_12:  max_mv = 3100; break;
        default: max_mv = 3300; break;
    }

    if (raw_val < 0) raw_val = 0;
    if (raw_val > max_raw) raw_val = max_raw;

    return (raw_val * max_mv) / max_raw;
}

// 2. ONESHOT РЕЖИМ (ONESHOT MODE)

static adc_drv_oneshot_channel_entry_t* adc_drv_oneshot_find_channel(adc_drv_oneshot_ctx_t *ctx, adc_channel_t chan) {
    if (!ctx) return NULL;
    for (size_t i = 0; i < ctx->channel_count; ++i) {
        if (ctx->channels[i].channel == chan && ctx->channels[i].configured) {
            return &ctx->channels[i];
        }
    }
    return NULL;
}

esp_err_t adc_drv_oneshot_init(adc_drv_oneshot_ctx_t *ctx, adc_unit_t unit, adc_oneshot_clk_src_t clk_src, adc_ulp_mode_t ulp_mode) {
    if (!ctx) return ESP_ERR_INVALID_ARG;

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = unit,
        .clk_src = clk_src,
        .ulp_mode = ulp_mode,
    };
    return adc_drv_oneshot_init_custom(ctx, &init_config);
}

esp_err_t adc_drv_oneshot_init_custom(adc_drv_oneshot_ctx_t *ctx, const adc_oneshot_unit_init_cfg_t *init_config) {
    if (!ctx || !init_config) return ESP_ERR_INVALID_ARG;

    memset(ctx, 0, sizeof(adc_drv_oneshot_ctx_t));
    ctx->unit = init_config->unit_id;

    esp_err_t ret = adc_oneshot_new_unit(init_config, &ctx->unit_handle);
    if (ret == ESP_OK) {
        ctx->is_initialized = true;
    } else {
        ctx->is_initialized = false;
        ESP_LOGE(TAG, "Failed to initialize Oneshot Unit %d: %s", ctx->unit, esp_err_to_name(ret));
    }
    return ret;
}

void adc_drv_oneshot_deinit(adc_drv_oneshot_ctx_t *ctx) {
    if (!ctx) return;

    for (size_t i = 0; i < ctx->channel_count; ++i) {
        if (ctx->channels[i].configured) {
            adc_drv_cali_deinit(&ctx->channels[i].cali_ctx);
            ctx->channels[i].configured = false;
        }
    }
    ctx->channel_count = 0;

    if (ctx->unit_handle) {
        adc_oneshot_del_unit(ctx->unit_handle);
        ctx->unit_handle = NULL;
    }
    ctx->is_initialized = false;
}

esp_err_t adc_drv_oneshot_config_channel(adc_drv_oneshot_ctx_t *ctx, const adc_drv_oneshot_chan_config_t *config) {
    if (!ctx || !config) return ESP_ERR_INVALID_ARG;

    if (!ctx->is_initialized || !ctx->unit_handle) {
        esp_err_t ret = adc_drv_oneshot_init(ctx, ctx->unit ? ctx->unit : ADC_UNIT_1, ADC_RTC_CLK_SRC_DEFAULT, ADC_ULP_MODE_DISABLE);
        if (ret != ESP_OK) return ret;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = config->atten,
        .bitwidth = config->bitwidth,
    };

    esp_err_t ret = adc_oneshot_config_channel(ctx->unit_handle, config->channel, &chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure channel %d: %s", config->channel, esp_err_to_name(ret));
        return ret;
    }

    adc_drv_oneshot_channel_entry_t *entry = adc_drv_oneshot_find_channel(ctx, config->channel);
    if (!entry) {
        if (ctx->channel_count >= ADC_DRV_MAX_CHANNELS) {
            return ESP_ERR_NO_MEM;
        }
        entry = &ctx->channels[ctx->channel_count++];
    }

    entry->channel = config->channel;
    entry->atten = config->atten;
    entry->bitwidth = config->bitwidth;
    entry->configured = true;

    if (config->enable_cali) {
        adc_drv_cali_init(&entry->cali_ctx, ctx->unit, config->channel, config->atten, config->bitwidth);
    } else {
        adc_drv_cali_deinit(&entry->cali_ctx);
    }

    return ESP_OK;
}

esp_err_t adc_drv_oneshot_config_channel_simple(adc_drv_oneshot_ctx_t *ctx, adc_channel_t chan, adc_atten_t atten, adc_bitwidth_t bitwidth, bool enable_cali) {
    adc_drv_oneshot_chan_config_t cfg = {
        .channel = chan,
        .atten = atten,
        .bitwidth = bitwidth,
        .enable_cali = enable_cali,
    };
    return adc_drv_oneshot_config_channel(ctx, &cfg);
}

esp_err_t adc_drv_oneshot_read_raw(adc_drv_oneshot_ctx_t *ctx, adc_channel_t chan, int *raw_out) {
    if (!ctx || !ctx->unit_handle || !raw_out) {
        return ESP_ERR_INVALID_ARG;
    }
    return adc_oneshot_read(ctx->unit_handle, chan, raw_out);
}

esp_err_t adc_drv_oneshot_read_voltage(adc_drv_oneshot_ctx_t *ctx,
                                        adc_channel_t chan,
                                        int *voltage_mv_out) {
    if (!ctx || !voltage_mv_out) {
        return ESP_ERR_INVALID_ARG;
    }

    int raw = 0;

    esp_err_t ret = adc_drv_oneshot_read_raw(ctx, chan, &raw);
    if (ret != ESP_OK) return ret;

    adc_drv_oneshot_channel_entry_t *entry = adc_drv_oneshot_find_channel(ctx, chan);
    if (entry) {
        return adc_drv_cali_raw_to_voltage(&entry->cali_ctx, raw, voltage_mv_out);
    }

    *voltage_mv_out = adc_drv_cali_estimate_voltage(raw, ADC_ATTEN_DB_12, ADC_BITWIDTH_DEFAULT);
    return ESP_OK;
}

esp_err_t adc_drv_oneshot_read_raw_average(adc_drv_oneshot_ctx_t *ctx,
                                            adc_channel_t chan,
                                            uint32_t samples_count,
                                            int *raw_out) {
    if (!ctx || !raw_out || samples_count == 0) return ESP_ERR_INVALID_ARG;

    uint32_t sum = 0;
    for (uint32_t i = 0; i < samples_count; ++i) {
        int val = 0;
        esp_err_t ret = adc_drv_oneshot_read_raw(ctx, chan, &val);
        if (ret != ESP_OK) return ret;
        sum += val;
    }

    *raw_out = (int)(sum / samples_count);
    return ESP_OK;
}

esp_err_t adc_drv_oneshot_read_voltage_average(adc_drv_oneshot_ctx_t *ctx,
                                                adc_channel_t chan,
                                                uint32_t samples_count,
                                                int *voltage_mv_out) {
    if (!ctx || !voltage_mv_out || samples_count == 0) return ESP_ERR_INVALID_ARG;

    int raw_avg = 0;
    esp_err_t ret = adc_drv_oneshot_read_raw_average(ctx, chan, samples_count, &raw_avg);
    if (ret != ESP_OK) return ret;

    adc_drv_oneshot_channel_entry_t *entry = adc_drv_oneshot_find_channel(ctx, chan);
    if (entry) {
        return adc_drv_cali_raw_to_voltage(&entry->cali_ctx, raw_avg, voltage_mv_out);
    }

    *voltage_mv_out = adc_drv_cali_estimate_voltage(raw_avg, ADC_ATTEN_DB_12, ADC_BITWIDTH_DEFAULT);
    return ESP_OK;
}

// 3. CONTINUOUS РЕЖИМ (CONTINUOUS / DMA MODE)

esp_err_t adc_drv_continuous_init_simple(adc_drv_continuous_ctx_t *ctx,
                                         const adc_channel_t *channels,
                                         size_t num_channels,
                                         uint32_t sample_freq_hz,
                                         uint32_t conv_frame_size,
                                         uint32_t max_store_buf_size,
                                         adc_unit_t unit,
                                         adc_atten_t atten,
                                         adc_bitwidth_t bitwidth) {
    if (!ctx || !channels || num_channels == 0 || num_channels > ADC_DRV_MAX_CHANNELS) {
        return ESP_ERR_INVALID_ARG;
    }

    adc_continuous_handle_cfg_t handle_cfg = {
        .max_store_buf_size = max_store_buf_size,
        .conv_frame_size = conv_frame_size,
        .flags = {},
    };

    adc_digi_pattern_config_t patterns[ADC_DRV_MAX_CHANNELS];
    for (size_t i = 0; i < num_channels; ++i) {
        patterns[i].atten = atten;
        patterns[i].channel = channels[i];
        patterns[i].unit = unit;
        patterns[i].bit_width = bitwidth;
    }

    adc_digi_convert_mode_t conv_mode = (unit == ADC_UNIT_1) ? ADC_CONV_SINGLE_UNIT_1 : ADC_CONV_SINGLE_UNIT_2;

    adc_continuous_config_t cont_cfg = {
        .pattern_num = (uint32_t)num_channels,
        .adc_pattern = patterns,
        .sample_freq_hz = sample_freq_hz,
        .conv_mode = conv_mode,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    };

    esp_err_t ret = adc_drv_continuous_init_custom(ctx, &handle_cfg, &cont_cfg);
    if (ret == ESP_OK) {
        for (size_t i = 0; i < num_channels; ++i) {
            adc_drv_cali_init(&ctx->cali_contexts[i], unit, channels[i], atten, bitwidth);
        }
        ctx->cali_count = num_channels;
    }
    return ret;
}

esp_err_t adc_drv_continuous_init_custom(adc_drv_continuous_ctx_t *ctx,
                                         const adc_continuous_handle_cfg_t *handle_cfg,
                                         const adc_continuous_config_t *cont_cfg) {
    if (!ctx || !handle_cfg || !cont_cfg) return ESP_ERR_INVALID_ARG;

    memset(ctx, 0, sizeof(adc_drv_continuous_ctx_t));

    esp_err_t ret = adc_continuous_new_handle(handle_cfg, &ctx->handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create Continuous ADC handle: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = adc_continuous_config(ctx->handle, cont_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure Continuous ADC: %s", esp_err_to_name(ret));
        adc_continuous_deinit(ctx->handle);
        ctx->handle = NULL;
        return ret;
    }

    return ESP_OK;
}

esp_err_t adc_drv_continuous_register_callbacks(adc_drv_continuous_ctx_t *ctx,
                                                const adc_continuous_evt_cbs_t *cbs,
                                                void *user_data) {
    if (!ctx || !ctx->handle || !cbs) return ESP_ERR_INVALID_ARG;
    return adc_continuous_register_event_callbacks(ctx->handle, cbs, user_data);
}

esp_err_t adc_drv_continuous_start(adc_drv_continuous_ctx_t *ctx) {
    if (!ctx || !ctx->handle) return ESP_ERR_INVALID_STATE;
    if (ctx->is_running) return ESP_OK;

    esp_err_t ret = adc_continuous_start(ctx->handle);
    if (ret == ESP_OK) {
        ctx->is_running = true;
    }
    return ret;
}

esp_err_t adc_drv_continuous_stop(adc_drv_continuous_ctx_t *ctx) {
    if (!ctx || !ctx->handle || !ctx->is_running) return ESP_OK;

    esp_err_t ret = adc_continuous_stop(ctx->handle);
    if (ret == ESP_OK) {
        ctx->is_running = false;
    }
    return ret;
}

esp_err_t adc_drv_continuous_read_raw_bytes(adc_drv_continuous_ctx_t *ctx,
                                            uint8_t *out_buf,
                                            uint32_t length_max,
                                            uint32_t *out_length_bytes,
                                            uint32_t timeout_ms) {
    if (!ctx || !ctx->handle || !out_buf || !out_length_bytes) {
        return ESP_ERR_INVALID_ARG;
    }
    return adc_continuous_read(ctx->handle, out_buf, length_max, out_length_bytes, pdMS_TO_TICKS(timeout_ms));
}

esp_err_t adc_drv_continuous_parse_samples(const adc_drv_continuous_ctx_t *ctx,
                                           const uint8_t *raw_bytes,
                                           uint32_t length_bytes,
                                           adc_drv_continuous_sample_t *out_samples,
                                           size_t max_samples,
                                           size_t *out_samples_count) {
    if (!raw_bytes || length_bytes == 0 || !out_samples || !out_samples_count) {
        return ESP_ERR_INVALID_ARG;
    }

    const uint32_t sample_size = SOC_ADC_DIGI_RESULT_BYTES;
    size_t count = 0;

    for (uint32_t i = 0; i + sample_size <= length_bytes && count < max_samples; i += sample_size) {
        const adc_digi_output_data_t *data = (const adc_digi_output_data_t *)&raw_bytes[i];
        adc_drv_continuous_sample_t *s = &out_samples[count++];

#if CONFIG_IDF_TARGET_ESP32
        s->channel = (adc_channel_t)data->type1.channel;
        s->raw_data = data->type1.data;
        s->unit = ADC_UNIT_1;
#else
        s->channel = (adc_channel_t)data->type2.channel;
        s->raw_data = data->type2.data;
        s->unit = (adc_unit_t)data->type2.unit;
#endif

        int voltage = 0;
        if (ctx && ctx->cali_count > 0) {
            adc_drv_cali_raw_to_voltage(&ctx->cali_contexts[0], s->raw_data, &voltage);
            s->voltage_mv = voltage;
        } else {
            s->voltage_mv = adc_drv_cali_estimate_voltage(s->raw_data, ADC_ATTEN_DB_12, ADC_BITWIDTH_DEFAULT);
        }
    }

    *out_samples_count = count;
    return ESP_OK;
}

void adc_drv_continuous_deinit(adc_drv_continuous_ctx_t *ctx) {
    if (!ctx) return;

    adc_drv_continuous_stop(ctx);

    for (size_t i = 0; i < ctx->cali_count; ++i) {
        adc_drv_cali_deinit(&ctx->cali_contexts[i]);
    }
    ctx->cali_count = 0;

    if (ctx->handle) {
        adc_continuous_deinit(ctx->handle);
        ctx->handle = NULL;
    }
    ctx->is_running = false;
}

// 4. ЗРУЧНИЙ ОДНОКАНАЛЬНИЙ ХЕЛПЕР (SINGLE CHANNEL HELPER)

esp_err_t adc_drv_channel_init(adc_drv_channel_ctx_t *ctx,
                               adc_unit_t unit,
                               adc_channel_t channel,
                               adc_atten_t atten,
                               adc_bitwidth_t bitwidth,
                               bool enable_cali) {
    if (!ctx) return ESP_ERR_INVALID_ARG;

    memset(ctx, 0, sizeof(adc_drv_channel_ctx_t));
    ctx->unit = unit;
    ctx->channel = channel;
    ctx->atten = atten;
    ctx->bitwidth = bitwidth;

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = unit,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    esp_err_t ret = adc_oneshot_new_unit(&init_config, &ctx->unit_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = atten,
        .bitwidth = bitwidth,
    };

    ret = adc_oneshot_config_channel(ctx->unit_handle, channel, &chan_cfg);
    if (ret != ESP_OK) {
        adc_oneshot_del_unit(ctx->unit_handle);
        ctx->unit_handle = NULL;
        return ret;
    }

    ctx->is_valid = true;
    if (enable_cali) {
        adc_drv_cali_init(&ctx->cali_ctx, unit, channel, atten, bitwidth);
    }
    return ESP_OK;
}

void adc_drv_channel_deinit(adc_drv_channel_ctx_t *ctx) {
    if (!ctx) return;

    adc_drv_cali_deinit(&ctx->cali_ctx);

    if (ctx->unit_handle) {
        adc_oneshot_del_unit(ctx->unit_handle);
        ctx->unit_handle = NULL;
    }
    ctx->is_valid = false;
}

int adc_drv_channel_read_raw(adc_drv_channel_ctx_t *ctx) {
    if (!ctx || !ctx->is_valid || !ctx->unit_handle) return -1;
    int raw = 0;
    if (adc_oneshot_read(ctx->unit_handle, ctx->channel, &raw) == ESP_OK) {
        return raw;
    }
    return -1;
}

int adc_drv_channel_read_voltage(adc_drv_channel_ctx_t *ctx) {
    int raw = adc_drv_channel_read_raw(ctx);
    if (raw < 0) return -1;

    int voltage = 0;
    if (adc_drv_cali_raw_to_voltage(&ctx->cali_ctx, raw, &voltage) == ESP_OK) {
        return voltage;
    }
    return adc_drv_cali_estimate_voltage(raw, ctx->atten, ctx->bitwidth);
}

int adc_drv_channel_read_raw_average(adc_drv_channel_ctx_t *ctx, uint32_t samples_count) {
    if (!ctx || !ctx->is_valid || samples_count == 0) return -1;

    uint32_t sum = 0;
    for (uint32_t i = 0; i < samples_count; ++i) {
        int r = adc_drv_channel_read_raw(ctx);
        if (r < 0) return -1;
        sum += r;
    }
    return (int)(sum / samples_count);
}

int adc_drv_channel_read_voltage_average(adc_drv_channel_ctx_t *ctx, uint32_t samples_count) {
    int raw_avg = adc_drv_channel_read_raw_average(ctx, samples_count);
    if (raw_avg < 0) return -1;

    int voltage = 0;
    if (adc_drv_cali_raw_to_voltage(&ctx->cali_ctx, raw_avg, &voltage) == ESP_OK) {
        return voltage;
    }
    return adc_drv_cali_estimate_voltage(raw_avg, ctx->atten, ctx->bitwidth);
}