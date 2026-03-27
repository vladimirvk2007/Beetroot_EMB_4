
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "esp_err.h"


extern "C" void app_main(void) {
    // 1. Конфігурація модуля (Unit)
    adc_oneshot_unit_handle_t adc1_handle;

    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // 2. Конфігурація каналу
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,         // Діапазон до ~3.1В
        .bitwidth = ADC_BITWIDTH_DEFAULT, // Зазвичай 12 біт
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_3, &config));

    // 3. (Опціонально) Налаштування калібрування для отримання мВ
    adc_cali_handle_t cali_handle = NULL;

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .chan = ADC_CHANNEL_3,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle));

    while (1) {
        int adc_raw;
        int voltage;

        // Зчитування сирого значення (0-4095)
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &adc_raw));

        // Перерахунок у мілівольти (якщо калібрування успішне)
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, adc_raw, &voltage));

        ESP_LOGI("ADC", "Raw: %d, Voltage: %d mV", adc_raw, voltage);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

