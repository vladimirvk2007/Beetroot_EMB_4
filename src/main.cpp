#include <algorithm>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

#include "driver/ledc.h"
#include "esp_err.h"


// PWM параметри
#define PWM_GPIO      18
#define PWM_FREQ_HZ   5000
#define PWM_RES_BITS  12
#define PWM_TIMER     LEDC_TIMER_0
#define PWM_MODE      LEDC_LOW_SPEED_MODE
#define PWM_CHANNEL   LEDC_CHANNEL_0


extern "C" void app_main(void) {
    // 1. Конфігурація модуля АЦП (Unit)
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

    // PWM (LEDC) ініціалізація
    ledc_timer_config_t ledc_timer = {
        .speed_mode = PWM_MODE,
        .duty_resolution = (ledc_timer_bit_t)PWM_RES_BITS,
        .timer_num = PWM_TIMER,
        .freq_hz = PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = 0
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .gpio_num       = PWM_GPIO,
        .speed_mode     = PWM_MODE,
        .channel        = PWM_CHANNEL,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = PWM_TIMER,
        .duty           = 0,
        .hpoint         = 0,
        .sleep_mode     = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags          = { .output_invert = 0 }
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    while (1) {
        int adc_raw;
        int voltage;

        // Зчитування сирого значення (0-4095)
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &adc_raw));

        // Перерахунок у мілівольти (якщо калібрування успішне)
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, adc_raw, &voltage));

        // Встановлення ширини імпульсу PWM (масштабування під 12 біт)
        uint32_t duty = std::min((uint32_t)adc_raw, (uint32_t)4095); // захист від виходу за межі
        ESP_ERROR_CHECK(ledc_set_duty(PWM_MODE, PWM_CHANNEL, duty));
        ESP_ERROR_CHECK(ledc_update_duty(PWM_MODE, PWM_CHANNEL));

        ESP_LOGI("ADC+PWM", "Raw: %d, Voltage: %d mV, PWM duty: %lu", adc_raw, voltage, duty);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

