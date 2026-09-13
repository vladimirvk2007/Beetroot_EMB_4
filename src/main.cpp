
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "adc.h"

#define ADC_IN_CHANNEL ADC_CHANNEL_3

static const char *TAG = "MAIN";

extern "C" void app_main(void) {
    // Oneshot context for ADC_UNIT_1
    adc_drv_oneshot_ctx_t adc_ctx;
    adc_drv_oneshot_init(&adc_ctx,
                        ADC_UNIT_1,
                        ADC_RTC_CLK_SRC_DEFAULT,
                        ADC_ULP_MODE_DISABLE);

    adc_drv_oneshot_config_channel_simple(&adc_ctx,
                                            ADC_IN_CHANNEL,
                                            ADC_ATTEN_DB_12,
                                            ADC_BITWIDTH_DEFAULT,
                                            true);

    while (1) {
        int raw_val = 0;
        int volt_mv = 0;
        int volt_avg_mv = 0;

        adc_drv_oneshot_read_raw(&adc_ctx, ADC_IN_CHANNEL, &raw_val);
        adc_drv_oneshot_read_voltage(&adc_ctx, ADC_IN_CHANNEL, &volt_mv);
        adc_drv_oneshot_read_voltage_average(&adc_ctx, ADC_IN_CHANNEL, 16, &volt_avg_mv);

        ESP_LOGI(TAG, "ADC Chan %d: Raw = %d, Voltage = %d mV (Avg16 = %d mV)",
                 ADC_IN_CHANNEL, raw_val, volt_mv, volt_avg_mv);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
