#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/pulse_cnt.h"
#include "esp_log.h"

#define ENCODER_A_INPUT         17
#define ENCODER_B_INPUT         16
#define ENCODER_BUTTON_INPUT    15

static const char *TAG = "ENCODER_APP";

// Функція ініціалізації
pcnt_unit_handle_t init_encoder(int gpio_a, int gpio_b) {
    pcnt_unit_config_t unit_config = {
        .low_limit = -32768,
        .high_limit = 32767,
        .intr_priority = 0,
        .flags = {
            .accum_count = 0,
        }
    };
    pcnt_unit_handle_t unit = NULL;
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &unit));

    // Фільтр брязкоту контактів
    pcnt_glitch_filter_config_t filter_config = { .max_glitch_ns = 1000 };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(unit, &filter_config));

    // Налаштування каналів
    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = gpio_a,
        .level_gpio_num = gpio_b,
        .flags = {
            .invert_edge_input = 0,
            .invert_level_input = 0,
            .virt_edge_io_level = 0,
            .virt_level_io_level = 0,
            .io_loop_back = 0,
        },
    };
    pcnt_channel_handle_t chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(unit, &chan_a_config, &chan_a));

    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = gpio_b,
        .level_gpio_num = gpio_a,
        .flags = {
            .invert_edge_input = 0,
            .invert_level_input = 0,
            .virt_edge_io_level = 0,
            .virt_level_io_level = 0,
            .io_loop_back = 0,
        },
    };
    pcnt_channel_handle_t chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(unit, &chan_b_config, &chan_b));

    // Квадратурна логіка
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(chan_a,
                                                PCNT_CHANNEL_EDGE_ACTION_DECREASE,
                                                PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(chan_a,
                                                PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                                PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(chan_b,
                                                PCNT_CHANNEL_EDGE_ACTION_INCREASE,
                                                PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(chan_b,
                                                PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                                PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_ERROR_CHECK(pcnt_unit_enable(unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(unit));
    ESP_ERROR_CHECK(pcnt_unit_start(unit));

    return unit;
}

extern "C" void app_main() {
    // Викликаємо ініціалізацію та отримуємо "дескриптор" лічильника
    pcnt_unit_handle_t encoder = init_encoder(ENCODER_A_INPUT, ENCODER_B_INPUT);

    int current_val = 0;
    while (1) {
        ESP_ERROR_CHECK(pcnt_unit_get_count(encoder, &current_val));
        ESP_LOGI(TAG, "Position: %d", current_val);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
