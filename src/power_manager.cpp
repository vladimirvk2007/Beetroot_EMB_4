#include "power_manager.h"
#include "esp_log.h"
#include "esp_pm.h"

static const char* TAG = "PM";

void configure_power_management() {
#if CONFIG_PM_ENABLE
#if !CONFIG_FREERTOS_USE_TICKLESS_IDLE
    ESP_LOGW(TAG,
             "CONFIG_FREERTOS_USE_TICKLESS_IDLE is disabled: automatic light sleep in idle will not work");
#endif

    // Allow DFS + automatic light sleep while FreeRTOS is idle.
    const esp_pm_config_t pm_config = {
        .max_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
        .min_freq_mhz = 40,
        .light_sleep_enable = true,
    };

    const esp_err_t err = esp_pm_configure(&pm_config);
    if (err == ESP_OK) {
        ESP_LOGI(TAG,
                 "Power management enabled: max=%d MHz min=%d MHz light_sleep=1",
                 pm_config.max_freq_mhz,
                 pm_config.min_freq_mhz);
    } else {
        ESP_LOGW(TAG, "esp_pm_configure failed: %s", esp_err_to_name(err));
    }
#else
    ESP_LOGW(TAG, "CONFIG_PM_ENABLE is disabled, light sleep in idle is unavailable");
#endif
}
