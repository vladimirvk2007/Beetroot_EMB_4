#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "led.h"
#include "button.h"

#define LED_OUT		GPIO_NUM_16
#define BUTTON_IN	GPIO_NUM_15

static constexpr uint64_t LIGHT_SLEEP_TIME_US = 5ULL * 1000ULL * 1000ULL;

static const char* wakeupCauseToString(esp_sleep_wakeup_cause_t cause) {
    switch (cause) {
        case ESP_SLEEP_WAKEUP_TIMER:
            return "TIMER";
        case ESP_SLEEP_WAKEUP_GPIO:
            return "GPIO";
        case ESP_SLEEP_WAKEUP_UNDEFINED:
            return "UNDEFINED (power-on/reset)";
        default:
            return "OTHER";
    }
}

extern "C" void app_main() {
    Led led(LED_OUT);
    Button button(BUTTON_IN);

    gpio_wakeup_enable(BUTTON_IN, GPIO_INTR_LOW_LEVEL); // Button is active-low.

    printf("Light-sleep demo started. Wakeup sources: TIMER (%" PRIu64 " us) and BUTTON GPIO %d.\n",
           LIGHT_SLEEP_TIME_US,
           static_cast<int>(BUTTON_IN));

    while (1) {
        if (button.isPressed()) {
            printf("Button is currently pressed. Release it to enter sleep.\n");
            vTaskDelay(pdMS_TO_TICKS(500));
            if (button.isPressed()) {
                continue; // Still pressed, wait longer
            }

            esp_sleep_enable_timer_wakeup(LIGHT_SLEEP_TIME_US);
            esp_sleep_enable_gpio_wakeup();

            led.off();
            printf("Entering light-sleep...\n");

            vTaskDelay(pdMS_TO_TICKS(50));

            esp_err_t err = esp_light_sleep_start();
            if (err != ESP_OK) {
                printf("esp_light_sleep_start failed: %s\n", esp_err_to_name(err));
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }

            esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
            printf("Woke up by: %s\n", wakeupCauseToString(cause));

            if (cause == ESP_SLEEP_WAKEUP_GPIO) {
                led.on();
                vTaskDelay(pdMS_TO_TICKS(200));
                led.off();
            }
        }   

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

