#ifndef LIB_ADC_ADC_H
#define LIB_ADC_ADC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "soc/soc_caps.h"
#include "hal/adc_types.h"

#define ADC_DRV_MAX_CHANNELS 10

// Контекст калібрування АЦП
typedef struct {
    adc_cali_handle_t cali_handle;
    adc_atten_t atten;
    adc_bitwidth_t bitwidth;
} adc_cali_t;

// Параметри налаштування каналу АЦП
typedef struct {
    adc_channel_t channel;
    adc_atten_t atten;
    adc_bitwidth_t bitwidth;
    adc_cali_t cali_ctx;
    bool configured;
} adc_oneshot_chan_t;

// Контекст АЦП
typedef struct {
    adc_unit_t unit;
    adc_oneshot_unit_handle_t unit_handle;
    adc_oneshot_chan_t channels[ADC_DRV_MAX_CHANNELS];
    size_t channel_count;
    bool is_initialized;
} adc_oneshot_ctx_t;

// Ініціалізація контексту калібрування
esp_err_t adc_cali_init(adc_cali_t *ctx,
                       adc_unit_t unit,
                       adc_channel_t chan,
                       adc_atten_t atten,
                       adc_bitwidth_t bitwidth);

// Ініціалізація контексту
esp_err_t adc_oneshot_init(adc_oneshot_ctx_t *ctx,
                            adc_unit_t unit,
                            adc_oneshot_clk_src_t clk_src,
                            adc_ulp_mode_t ulp_mode);

// Зчитування значення АЦП з каналу
esp_err_t adc_oneshot_read_raw(adc_oneshot_ctx_t *ctx,
                               adc_channel_t chan,
                               int *raw_out);

// Зчитування напруги (мВ) з урахуванням калібрування
esp_err_t adc_oneshot_read_voltage(adc_oneshot_ctx_t *ctx,
                                   adc_channel_t chan,
                                   int *voltage_mv_out);

// Просте налаштування каналу
esp_err_t adc_oneshot_config(adc_oneshot_ctx_t *ctx,
                             adc_channel_t chan,
                             adc_atten_t atten,
                             adc_bitwidth_t bitwidth,
                             bool enable_cali);

// Перерахунок значення АЦП у напругу (мВ) з урахуванням калібрування
esp_err_t adc_raw_to_mv(const adc_cali_t *ctx, int raw_val,
                                            int *voltage_mv_out);

// Перерахунок значення АЦП у напругу без апаратного калібрування
int adc_est_voltage(int raw_val, adc_atten_t atten, adc_bitwidth_t bitwidth);

// Звільнення ресурсів калібрування
void adc_cali_deinit(adc_cali_t *ctx);

// Деініціалізація та звільнення ресурсів
void adc_oneshot_deinit(adc_oneshot_ctx_t *ctx);

#endif // LIB_ADC_ADC_H
