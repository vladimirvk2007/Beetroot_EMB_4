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


// 3. CONTINUOUS РЕЖИМ (CONTINUOUS / DMA MODE)

typedef struct {
    adc_unit_t unit;
    adc_channel_t channel;
    uint32_t raw_data;
    int voltage_mv;
} adc_drv_continuous_sample_t;

typedef struct {
    adc_continuous_handle_t handle;
    bool is_running;
    adc_drv_cali_ctx_t cali_contexts[ADC_DRV_MAX_CHANNELS];
    size_t cali_count;
} adc_drv_continuous_ctx_t;

// Спрощена ініціалізація Continuous DMA АЦП
esp_err_t adc_drv_continuous_init_simple(adc_drv_continuous_ctx_t *ctx,
                                         const adc_channel_t *channels,
                                         size_t num_channels,
                                         uint32_t sample_freq_hz,
                                         uint32_t conv_frame_size,
                                         uint32_t max_store_buf_size,
                                         adc_unit_t unit,
                                         adc_atten_t atten,
                                         adc_bitwidth_t bitwidth);

// Повна ініціалізація Continuous DMA з кастомними структурами ESP-IDF
esp_err_t adc_drv_continuous_init_custom(adc_drv_continuous_ctx_t *ctx,
                                         const adc_continuous_handle_cfg_t *handle_cfg,
                                         const adc_continuous_config_t *cont_cfg);

// Реєстрація зворотних викликів (callbacks) DMA
esp_err_t adc_drv_continuous_register_callbacks(adc_drv_continuous_ctx_t *ctx,
                                                const adc_continuous_evt_cbs_t *cbs,
                                                void *user_data);

// Запуск DMA перетворень
esp_err_t adc_drv_continuous_start(adc_drv_continuous_ctx_t *ctx);

// Зупинка DMA перетворень
esp_err_t adc_drv_continuous_stop(adc_drv_continuous_ctx_t *ctx);

// Зчитування сирого пулу байтів з буфера DMA
esp_err_t adc_drv_continuous_read_raw_bytes(adc_drv_continuous_ctx_t *ctx,
                                            uint8_t *out_buf,
                                            uint32_t length_max,
                                            uint32_t *out_length_bytes,
                                            uint32_t timeout_ms);

// Парсинг отриманого DMA-буфера у структуровані вибірки з перерахунком у напругу (мВ)
esp_err_t adc_drv_continuous_parse_samples(const adc_drv_continuous_ctx_t *ctx,
                                           const uint8_t *raw_bytes,
                                           uint32_t length_bytes,
                                           adc_drv_continuous_sample_t *out_samples,
                                           size_t max_samples,
                                           size_t *out_samples_count);

// Деініціалізація Continuous контексту.
void adc_drv_continuous_deinit(adc_drv_continuous_ctx_t *ctx);


// 4. ЗРУЧНИЙ ОДНОКАНАЛЬНИЙ ХЕЛПЕР (SINGLE CHANNEL HELPER)

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
