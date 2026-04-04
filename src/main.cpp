#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define ENCODER_A_INPUT         17
#define ENCODER_B_INPUT         16
#define ENCODER_BUTTON_INPUT    15
#define ENCODER_DEBOUNCE_NS     1000
#define ENCODER_MAX_COUNT       32767
#define ENCODER_MIN_COUNT       -32768

#define LED_OUTPUT              4

static const char *TAG = "Encoder";

// Функція ініціалізації
pcnt_unit_handle_t init_encoder(int gpio_a, int gpio_b) {
    pcnt_unit_config_t unit_config = {
        .low_limit = ENCODER_MIN_COUNT,
        .high_limit = ENCODER_MAX_COUNT,
        .intr_priority = 0,
        .flags = {
            .accum_count = 0,
        }
    };
    pcnt_unit_handle_t unit = NULL;
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &unit));

    // Фільтр брязкоту контактів
    pcnt_glitch_filter_config_t filter_config = { .max_glitch_ns = ENCODER_DEBOUNCE_NS };
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

    // Ініціалізація GPIO для кнопки та світлодіода
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ENCODER_BUTTON_INPUT) | (1ULL << LED_OUTPUT),
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_direction((gpio_num_t)ENCODER_BUTTON_INPUT, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t)ENCODER_BUTTON_INPUT, GPIO_PULLUP_ONLY);
    gpio_set_direction((gpio_num_t)LED_OUTPUT, GPIO_MODE_OUTPUT);

    int current_val = 0;
    while (1) {
        // Читаємо поточне значення лічильника енкодера
        ESP_ERROR_CHECK(pcnt_unit_get_count(encoder, &current_val));

        // Читаємо стани входів A, B
        int a_state = gpio_get_level((gpio_num_t)ENCODER_A_INPUT);
        int b_state = gpio_get_level((gpio_num_t)ENCODER_B_INPUT);

        ESP_LOGI(TAG, "Position: %d | A: %d | B: %d", current_val, a_state, b_state);

        // Читаємо стан кнопки та керуємо світлодіодом
        int btn_state = gpio_get_level((gpio_num_t)ENCODER_BUTTON_INPUT);
        if (btn_state == 0) { // натиснуто (активний low)
            gpio_set_level((gpio_num_t)LED_OUTPUT, 1);
        } else {
            gpio_set_level((gpio_num_t)LED_OUTPUT, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
