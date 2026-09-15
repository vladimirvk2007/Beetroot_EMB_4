#include <string.h>
#include "adc.h"

static const char *TAG = "ADC_DRIVER";

// Пошук каналу в контексті
static adc_oneshot_chan_t* adc_find_chan(adc_oneshot_ctx_t *ctx,
                                                adc_channel_t chan) {
    if (ctx == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < ctx->channel_count; ++i) {
        if (ctx->channels[i].channel == chan && ctx->channels[i].configured) {
            return &ctx->channels[i];
        }
    }

    return NULL;
}

// Налаштування каналу АЦП
static esp_err_t adc_config_chan(adc_oneshot_ctx_t *ctx,
                                adc_channel_t chan,
                                adc_atten_t atten,
                                adc_bitwidth_t bitwidth,
                                bool enable_cali) {
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!ctx->is_initialized || ctx->unit_handle == NULL) {
        esp_err_t ret = adc_oneshot_init(ctx, ctx->unit ? ctx->unit : ADC_UNIT_1,
                                                        ADC_RTC_CLK_SRC_DEFAULT,
                                                        ADC_ULP_MODE_DISABLE);
        if (ret != ESP_OK) {
            return ret;
        }
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = atten,
        .bitwidth = bitwidth,
    };

    esp_err_t ret = adc_oneshot_config_channel(ctx->unit_handle, chan, &chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure channel %d: %s", chan, esp_err_to_name(ret));
        return ret;
    }

    adc_oneshot_chan_t *entry = adc_find_chan(ctx, chan);
    if (entry == NULL) {
        if (ctx->channel_count >= ADC_DRV_MAX_CHANNELS) {
            return ESP_ERR_NO_MEM;
        }
        entry = &ctx->channels[ctx->channel_count++];
    }

    entry->channel = chan;
    entry->atten = atten;
    entry->bitwidth = bitwidth;
    entry->configured = true;

    if (enable_cali) {
        adc_cali_init(&entry->cali_ctx, ctx->unit, chan, atten, bitwidth);
    } else {
        adc_cali_deinit(&entry->cali_ctx);
    }

    return ESP_OK;
}

// Ініціалізація контексту калібрування
esp_err_t adc_cali_init(adc_cali_t *ctx,
                       adc_unit_t unit, adc_channel_t chan,
                       adc_atten_t atten,
                       adc_bitwidth_t bitwidth) {
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(ctx, 0, sizeof(adc_cali_t));
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

// Ініціалізація контексту
esp_err_t adc_oneshot_init(adc_oneshot_ctx_t *ctx,
                            adc_unit_t unit,
                            adc_oneshot_clk_src_t clk_src,
                            adc_ulp_mode_t ulp_mode) {
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = unit,
        .clk_src = clk_src,
        .ulp_mode = ulp_mode,
    };

    memset(ctx, 0, sizeof(adc_oneshot_ctx_t));
    ctx->unit = unit;

    esp_err_t ret = adc_oneshot_new_unit(&init_config, &ctx->unit_handle);
    if (ret == ESP_OK) {
        ctx->is_initialized = true;
        return ESP_OK;
    }

    ctx->is_initialized = false;
    ESP_LOGE(TAG, "Failed to initialize Oneshot Unit %d: %s", unit, esp_err_to_name(ret));

    return ret;
}

// Зчитування значення АЦП з каналу
esp_err_t adc_oneshot_read_raw(adc_oneshot_ctx_t *ctx,
                               adc_channel_t chan,
                               int *raw_out) {
    if (ctx == NULL || ctx->unit_handle == NULL || raw_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    return adc_oneshot_read(ctx->unit_handle, chan, raw_out);
}

// Зчитування напруги (мВ) з урахуванням калібрування
esp_err_t adc_oneshot_read_voltage(adc_oneshot_ctx_t *ctx,
                                   adc_channel_t chan,
                                   int *voltage_mv_out) {
    if (ctx == NULL || ctx->unit_handle == NULL || voltage_mv_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int raw = 0;
    esp_err_t ret = adc_oneshot_read_raw(ctx, chan, &raw);
    if (ret != ESP_OK) {
        return ret;
    }

    adc_oneshot_chan_t *entry = adc_find_chan(ctx, chan);
    if (entry != NULL) {
        return adc_raw_to_mv(&entry->cali_ctx, raw, voltage_mv_out);
    }

    *voltage_mv_out = adc_est_voltage(raw, ADC_ATTEN_DB_12, ADC_BITWIDTH_DEFAULT);

    return ESP_OK;
}

// Просте налаштування каналу
esp_err_t adc_oneshot_config(adc_oneshot_ctx_t *ctx,
                             adc_channel_t chan,
                             adc_atten_t atten,
                             adc_bitwidth_t bitwidth,
                             bool enable_cali) {
    return adc_config_chan(ctx, chan, atten, bitwidth, enable_cali);
}

// Перерахунок значення АЦП у напругу (мВ) з урахуванням калібрування
esp_err_t adc_raw_to_mv(const adc_cali_t *ctx,
                        int raw_val,
                        int *voltage_mv_out) {
    if (!ctx || !voltage_mv_out) {
        return ESP_ERR_INVALID_ARG;
    }

    if (ctx->cali_handle != NULL) {
        return adc_cali_raw_to_voltage(ctx->cali_handle, raw_val, voltage_mv_out);
    }

    *voltage_mv_out = adc_est_voltage(raw_val, ctx->atten, ctx->bitwidth);

    return ESP_OK;
}

// Перерахунок значення АЦП у напругу без апаратного калібрування
int adc_est_voltage(int raw_val,
                   adc_atten_t atten,
                   adc_bitwidth_t bitwidth) {
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

// Звільнення ресурсів калібрування
void adc_cali_deinit(adc_cali_t *ctx) {
    if (ctx == NULL) {
        return;
    }

    if (ctx->cali_handle) {
        adc_cali_delete_scheme_curve_fitting(ctx->cali_handle);
        ctx->cali_handle = NULL;
    }
}

// Деініціалізація та звільнення ресурсів
void adc_oneshot_deinit(adc_oneshot_ctx_t *ctx) {
    if (ctx == NULL) {
        return;
    }

    for (size_t i = 0; i < ctx->channel_count; ++i) {
        if (ctx->channels[i].configured) {
            adc_cali_deinit(&ctx->channels[i].cali_ctx);
            ctx->channels[i].configured = false;
        }
    }
    ctx->channel_count = 0;

    if (ctx->unit_handle != NULL) {
        adc_oneshot_del_unit(ctx->unit_handle);
        ctx->unit_handle = NULL;
    }
    ctx->is_initialized = false;
}
