#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include "led.h"
#include "button.h"

#define LED_OUT		GPIO_NUM_16
#define BUTTON_IN	GPIO_NUM_15

static constexpr uint64_t DEEP_SLEEP_TIME_US = 10 * 1000 * 1000;

static const char* wakeupCauseToString(esp_sleep_wakeup_cause_t cause) {
    switch (cause) {
        case ESP_SLEEP_WAKEUP_TIMER:
            return "TIMER";
        case ESP_SLEEP_WAKEUP_EXT0:
            return "EXT0 (BUTTON)";
        case ESP_SLEEP_WAKEUP_EXT1:
            return "EXT1";
        case ESP_SLEEP_WAKEUP_UNDEFINED:
            return "UNDEFINED (power-on/reset)";
        default:
            return "OTHER";
    }
}

extern "C" void app_main() {
    Led led(LED_OUT);
    Button button(BUTTON_IN);

    led.on();

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    printf("Boot. Wakeup cause: %s\n", wakeupCauseToString(cause));

    printf("Deep-sleep demo started.\n");
    printf("Wakeup sources: TIMER (%" PRIu64 " us) and BUTTON GPIO %d (EXT0, active LOW).\n",
           DEEP_SLEEP_TIME_US,
           static_cast<int>(BUTTON_IN));
    printf("Press button to enter deep-sleep.\n");

    rtc_gpio_pullup_en(BUTTON_IN);
    rtc_gpio_pulldown_dis(BUTTON_IN);

    while (1) {
        if (button.isPressed()) {
            vTaskDelay(pdMS_TO_TICKS(100));

            esp_err_t timerErr = esp_sleep_enable_timer_wakeup(DEEP_SLEEP_TIME_US);
            if (timerErr != ESP_OK) {
                printf("Failed to set timer wakeup: %s\n", esp_err_to_name(timerErr));
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }
            
            esp_err_t ext0Err = esp_sleep_enable_ext0_wakeup(BUTTON_IN, 0);
            if (ext0Err != ESP_OK) {
                printf("Failed to set EXT0 wakeup: %s\n", esp_err_to_name(ext0Err));
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }

            printf("Entering deep-sleep...\n");
            vTaskDelay(pdMS_TO_TICKS(100));

            esp_deep_sleep_start();
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

