
#include <algorithm>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_task_wdt.h"
#include "driver/gptimer.h"
#include "driver/gpio.h"
// LED GPIO
#define LED_GPIO 4

// PWM параметри
#define PWM_GPIO      18
#define PWM_RES_BITS  12
#define PWM_TIMER     LEDC_TIMER_0
#define PWM_MODE      LEDC_LOW_SPEED_MODE
#define PWM_CHANNEL   LEDC_CHANNEL_0

// Визначення нот (частоти в Гц)
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523

// Кількість точок у таблиці синуса
#define SINE_TABLE_SIZE 64

// Масив нот для програвання
const int notes[] = { NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5 };
const int notes_count = sizeof(notes) / sizeof(notes[0]);

// Коректна синусоїда для 12-бітного PWM
const uint16_t sine_table[SINE_TABLE_SIZE] = {
    2048, 2248, 2447, 2642, 2831, 3013, 3185, 3347,
    3496, 3631, 3750, 3854, 3939, 4007, 4056, 4085,
    4095, 4085, 4056, 4007, 3939, 3854, 3750, 3631,
    3496, 3347, 3185, 3013, 2831, 2642, 2447, 2248,
    2048, 1847, 1648, 1453, 1264, 1082, 910, 748,
    599, 464, 345, 241, 156, 88, 39, 10,
    0, 10, 39, 88, 156, 241, 345, 464,
    599, 748, 910, 1082, 1264, 1453, 1648, 1847
};

// Функція для відтворення однієї ноти синусом
void play_note(int freq_hz, int duration_ms) {
    // Налаштування таймера PWM на частоту 18 кГц
    int pwm_freq = 18000;
    ledc_timer_config_t ledc_timer = {
        .speed_mode = PWM_MODE,
        .duty_resolution = (ledc_timer_bit_t)PWM_RES_BITS,
        .timer_num = PWM_TIMER,
        .freq_hz = (uint32_t)pwm_freq,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = 0
    };
    ledc_timer_config(&ledc_timer);

    // Кількість семплів синуса на період
    int samples_per_period = SINE_TABLE_SIZE;
    // Період синуса в мкс
    double period_us = 1000000.0 / freq_hz;
    // Час одного семплу (мкс)
    double sample_time_us = period_us / samples_per_period;
    // Кількість ітерацій
    int total_samples = (int)(duration_ms * 1000 / sample_time_us);

    for (int i = 0; i < total_samples; i++) {
        int idx = i % SINE_TABLE_SIZE;
        ledc_set_duty(PWM_MODE, PWM_CHANNEL, sine_table[idx]);
        ledc_update_duty(PWM_MODE, PWM_CHANNEL);
        // Точна затримка в мікросекундах
        esp_rom_delay_us((uint32_t)sample_time_us);
    }
    // Вимкнути звук (duty = 0)
    ledc_set_duty(PWM_MODE, PWM_CHANNEL, 0);
    ledc_update_duty(PWM_MODE, PWM_CHANNEL);
}

extern "C" void app_main(void) {
    // Відключення Task Watchdog (ESP-IDF)
    esp_task_wdt_deinit();

    // PWM (LEDC) ініціалізація (канал)
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

    // LED GPIO init
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    int led_state = 0;

    while (1) {
        for (int i = 0; i < notes_count; ++i) {
            play_note(notes[i], 1000);

            // Переключення світлодіода після кожної ноти
            led_state = !led_state;
            gpio_set_level((gpio_num_t)LED_GPIO, led_state);

            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

