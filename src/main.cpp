#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"

#define LED_OUT		GPIO_NUM_16
#define BUTTON_IN	GPIO_NUM_15
#define ADC_CHANNEL	ADC_CHANNEL_3 // ADC1_CH3: GPIO4


extern "C" void app_main() {

    esp_err_t err = ESP_OK;

    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t adc_unit_config = {};
    adc_unit_config.unit_id = ADC_UNIT_1;
    err = adc_oneshot_new_unit(&adc_unit_config, &adc_handle);
    if (err != ESP_OK) {
        printf("Failed to initialize ADC, err = %d\n", err);
        return;
    }

    adc_oneshot_chan_cfg_t adc_channel_config = {};
    adc_channel_config.bitwidth = ADC_BITWIDTH_DEFAULT;
    adc_channel_config.atten = ADC_ATTEN_DB_12;
    err = adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &adc_channel_config);
    if (err != ESP_OK) {
        printf("Failed to configure ADC channel, err = %d\n", err);
        return;
    }

    adc_cali_handle_t calibration_handle = nullptr;
    adc_cali_curve_fitting_config_t calibration_config = {};
    calibration_config.unit_id = ADC_UNIT_1;
    calibration_config.chan = ADC_CHANNEL;
    calibration_config.atten = ADC_ATTEN_DB_12;
    calibration_config.bitwidth = ADC_BITWIDTH_DEFAULT;
    bool calibration_enabled =
        adc_cali_create_scheme_curve_fitting(&calibration_config, &calibration_handle) == ESP_OK;

    // Налаштування структури GPIO для LED
    gpio_config_t gpio_led_conf = {};
    gpio_led_conf.pin_bit_mask = 1ULL << LED_OUT;
    gpio_led_conf.mode = GPIO_MODE_OUTPUT;
    gpio_led_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_led_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_led_conf.intr_type = GPIO_INTR_DISABLE;


    // Конфігурація GPIO
    gpio_config(&gpio_led_conf);

    // Встановлення початкового стану
    gpio_set_level(LED_OUT, 0);

    while (1) {
        static bool led_state = 0;
        int adc_raw = 0;

        err = adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_raw);
        if (err == ESP_OK) {
            if (calibration_enabled) {
                int voltage_mv = 0;
                adc_cali_raw_to_voltage(calibration_handle, adc_raw, &voltage_mv);
                printf("ADC GPIO4: raw=%d, voltage=%d mV\n", adc_raw, voltage_mv);
            } else {
                printf("ADC GPIO4 raw: %d (calibration unavailable)\n", adc_raw);
            }
        } else {
            printf("Failed to read ADC, err = %d\n", err);
        }

        led_state = !led_state;
        gpio_set_level(LED_OUT, led_state);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
