
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "adc.h"

#define ADC_IN_CHANNEL_1 ADC_CHANNEL_3  // GPIO4
#define ADC_IN_CHANNEL_2 ADC_CHANNEL_4  // GPIO5

static const char *TAG = "MAIN";

extern "C" void app_main(void) {
    // Oneshot context for ADC_UNIT_1
    adc_oneshot_ctx_t adc_ctx;
    adc_oneshot_init(&adc_ctx,
                     ADC_UNIT_1,
                     ADC_RTC_CLK_SRC_DEFAULT,
                     ADC_ULP_MODE_DISABLE);

    // Channel 1 configuration (GPIO4 on ESP32-S3)
    adc_oneshot_config(&adc_ctx,
                       ADC_IN_CHANNEL_1,
                       ADC_ATTEN_DB_12,
                       ADC_BITWIDTH_DEFAULT,
                       true);

    // Channel 2 configuration (GPIO5 on ESP32-S3)
    adc_oneshot_config(&adc_ctx,
                       ADC_IN_CHANNEL_2,
                       ADC_ATTEN_DB_12,
                       ADC_BITWIDTH_DEFAULT,
                       true);

    while (1) {
        // Channel 1 measurements
        int raw_ch1 = 0;
        int volt_ch1 = 0;
        adc_oneshot_read_raw(&adc_ctx, ADC_IN_CHANNEL_1, &raw_ch1);
        adc_oneshot_read_voltage(&adc_ctx, ADC_IN_CHANNEL_1, &volt_ch1);

        // Channel 2 measurements
        int raw_ch2 = 0;
        int volt_ch2 = 0;
        adc_oneshot_read_raw(&adc_ctx, ADC_IN_CHANNEL_2, &raw_ch2);
        adc_oneshot_read_voltage(&adc_ctx, ADC_IN_CHANNEL_2, &volt_ch2);

        ESP_LOGI(TAG, "CH%d: Raw=%d, Volt=%d mV | CH%d: Raw=%d, Volt=%d mV",
                 ADC_IN_CHANNEL_1, raw_ch1, volt_ch1,
                 ADC_IN_CHANNEL_2, raw_ch2, volt_ch2);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
