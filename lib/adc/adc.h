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

// 1. КАЛІБРУВАННЯ

typedef struct {
    adc_cali_handle_t cali_handle;
    adc_atten_t atten;
    adc_bitwidth_t bitwidth;
} adc_drv_cali_ctx_t;

// Ініціалізація контексту калібрування

esp_err_t adc_drv_cali_init(adc_drv_cali_ctx_t *ctx,
                            adc_unit_t unit,
                            adc_channel_t chan,
                            adc_atten_t atten,
                            adc_bitwidth_t bitwidth);

// Звільнення ресурсів калібрування
void adc_drv_cali_deinit(adc_drv_cali_ctx_t *ctx);

// Перерахунок сирого значення у напругу (мВ)
esp_err_t adc_drv_cali_raw_to_voltage(const adc_drv_cali_ctx_t *ctx,
                                        int raw_val,
                                        int *voltage_mv_out);

// Математична оцінка напруги без апаратного калібрування
int adc_drv_cali_estimate_voltage(int raw_val,
                                    adc_atten_t atten,
                                    adc_bitwidth_t bitwidth);


// 2. ONESHOT РЕЖИМ
typedef struct {
    adc_channel_t channel;
    adc_atten_t atten;
    adc_bitwidth_t bitwidth;
    bool enable_cali;
} adc_drv_oneshot_chan_config_t;

typedef struct {
    adc_channel_t channel;
    adc_atten_t atten;
    adc_bitwidth_t bitwidth;
    adc_drv_cali_ctx_t cali_ctx;
    bool configured;
} adc_drv_oneshot_channel_entry_t;

typedef struct {
    adc_unit_t unit;
    adc_oneshot_unit_handle_t unit_handle;
    adc_drv_oneshot_channel_entry_t channels[ADC_DRV_MAX_CHANNELS];
    size_t channel_count;
    bool is_initialized;
} adc_drv_oneshot_ctx_t;

// Ініціалізація контексту Oneshot АЦП (з базовими параметрами)
esp_err_t adc_drv_oneshot_init(adc_drv_oneshot_ctx_t *ctx,
                                adc_unit_t unit,
                                adc_oneshot_clk_src_t clk_src,
                                adc_ulp_mode_t ulp_mode);

// Ініціалізація з користувацькою структурою ESP-IDF adc_oneshot_unit_init_cfg_t
esp_err_t adc_drv_oneshot_init_custom(adc_drv_oneshot_ctx_t *ctx,
                                        const adc_oneshot_unit_init_cfg_t *init_config);

// Деініціалізація та звільнення ресурсів блоку Oneshot
void adc_drv_oneshot_deinit(adc_drv_oneshot_ctx_t *ctx);

// Конфігурація конкретного каналу в блоці Oneshot
esp_err_t adc_drv_oneshot_config_channel(adc_drv_oneshot_ctx_t *ctx,
                                            const adc_drv_oneshot_chan_config_t *config);

// Швидка конфігурація каналу
esp_err_t adc_drv_oneshot_config_channel_simple(adc_drv_oneshot_ctx_t *ctx,
                                                adc_channel_t chan,
                                                adc_atten_t atten,
                                                adc_bitwidth_t bitwidth,
                                                bool enable_cali);

// Зчитування сирого значення
esp_err_t adc_drv_oneshot_read_raw(adc_drv_oneshot_ctx_t *ctx,
                                    adc_channel_t chan,
                                    int *raw_out);

// Зчитування напруги (мВ) з урахуванням калібрування
esp_err_t adc_drv_oneshot_read_voltage(adc_drv_oneshot_ctx_t *ctx,
                                        adc_channel_t chan,
                                        int *voltage_mv_out);

// Зчитування усередненого сирого значення (фільтрація шуму)
esp_err_t adc_drv_oneshot_read_raw_average(adc_drv_oneshot_ctx_t *ctx,
                                            adc_channel_t chan,
                                            uint32_t samples_count,
                                            int *raw_out);

// Зчитування усередненої напруги (мВ)
esp_err_t adc_drv_oneshot_read_voltage_average(adc_drv_oneshot_ctx_t *ctx,
                                                adc_channel_t chan,
                                                uint32_t samples_count,
                                                int *voltage_mv_out);

// 3. ЗРУЧНИЙ ОДНОКАНАЛЬНИЙ ХЕЛПЕР (SINGLE CHANNEL HELPER)

typedef struct {
    adc_unit_t unit;
    adc_channel_t channel;
    adc_atten_t atten;
    adc_bitwidth_t bitwidth;
    adc_oneshot_unit_handle_t unit_handle;
    adc_drv_cali_ctx_t cali_ctx;
    bool is_valid;
} adc_drv_channel_ctx_t;

// Ініціалізація окремого каналу АЦП
esp_err_t adc_drv_channel_init(adc_drv_channel_ctx_t *ctx,
                               adc_unit_t unit,
                               adc_channel_t channel,
                               adc_atten_t atten,
                               adc_bitwidth_t bitwidth,
                               bool enable_cali);

// Звільнення ресурсів каналу
void adc_drv_channel_deinit(adc_drv_channel_ctx_t *ctx);

// Зчитування сирого значення з каналу
int adc_drv_channel_read_raw(adc_drv_channel_ctx_t *ctx);

// Зчитування напруги в мілівольтах з каналу
int adc_drv_channel_read_voltage(adc_drv_channel_ctx_t *ctx);

// Зчитування усередненого сирого значення
int adc_drv_channel_read_raw_average(adc_drv_channel_ctx_t *ctx,
                                        uint32_t samples_count);

// Зчитування усередненої напруги в мВ
int adc_drv_channel_read_voltage_average(adc_drv_channel_ctx_t *ctx,
                                            uint32_t samples_count);

#endif // LIB_ADC_ADC_H
