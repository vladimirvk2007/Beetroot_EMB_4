#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "encoder.h"

#define ENCODER_A_INPUT      GPIO_NUM_17
#define ENCODER_B_INPUT      GPIO_NUM_16
#define ENCODER_BUTTON_INPUT GPIO_NUM_15
#define ENCODER_DEBOUNCE_NS  1000

static const char *TAG = "Encoder";

extern "C" void app_main(void) {
    encoder_ctx_t encoder;
    int32_t last_pulses = 0;
    bool last_button_pressed = false;
    bool initial_log_sent = false;

    ESP_ERROR_CHECK(encoder_init(&encoder,
                                ENCODER_A_INPUT,
                                ENCODER_B_INPUT,
                                ENCODER_BUTTON_INPUT,
                                ENCODER_DEBOUNCE_NS));

    while (1) {
        int32_t pulses = 0;
        bool button_pressed = false;

        ESP_ERROR_CHECK(encoder_get_pulses(&encoder, &pulses));
        ESP_ERROR_CHECK(encoder_get_button(&encoder, &button_pressed));

        int a_state = gpio_get_level(ENCODER_A_INPUT);
        int b_state = gpio_get_level(ENCODER_B_INPUT);

        const bool event_occurred = !initial_log_sent ||
                                   pulses != last_pulses ||
                                   button_pressed != last_button_pressed;

        if (event_occurred) {
            ESP_LOGI(TAG, "Pulses: %ld | A: %d | B: %d | Button: %d",
                     (long)pulses, a_state, b_state, button_pressed ? 1 : 0);
            last_pulses = pulses;
            last_button_pressed = button_pressed;
            initial_log_sent = true;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
