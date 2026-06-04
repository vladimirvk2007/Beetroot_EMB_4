#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "debounce.h"

#define BUTTON_PIN GPIO_NUM_15
#define LED_PIN GPIO_NUM_16

#define TASK_STACK_WORDS 3072
#define LED_TASK_CORE 1
#define HEARTBEAT_TASK_CORE 0
#define HEARTBEAT_TASK_PERIOD_MS 1000
#define BUTTON_TASK_PERIOD_MS 5
#define DEBOUNCE_TIME_MS 50
#define PRESS_SIGNAL_MAX_COUNT 32

static const char* TAG_LED_TASK = "LED_TASK";
static const char* TAG_HEARTBEAT = "HEARTBEAT";
static const char* TAG_APP_MAIN = "APP_MAIN";

static SemaphoreHandle_t g_press_semaphore = nullptr;
static uint32_t g_press_counter = 0;
static bool g_led_on = false;

// LED task отримує подію натискання кнопки, інкрементує глобальний лічильник і перемикає LED.
static void led_task(void* pvParameters) {
    (void)pvParameters;

    Debounce buttonDebounce(true, DEBOUNCE_TIME_MS);

    while (1) {
        bool rawReleased = gpio_get_level(BUTTON_PIN) != 0;
        uint32_t nowMs = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);

        if (buttonDebounce.update(rawReleased, nowMs) && !buttonDebounce.state()) {
            g_press_counter++;
            uint32_t local_counter = g_press_counter;

            g_led_on = !g_led_on;
            gpio_set_level(LED_PIN, g_led_on ? 1 : 0);

            BaseType_t signalSent = xSemaphoreGive(g_press_semaphore);
            if (signalSent != pdTRUE) {
                ESP_LOGW(TAG_LED_TASK, "Semaphore full, signal dropped");
            } else {
                ESP_LOGI(TAG_LED_TASK, "Button press -> counter=%lu, LED=%s, signal=sent",
                         static_cast<unsigned long>(local_counter),
                         g_led_on ? "ON" : "OFF");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Heartbeat task чекає сигнал від LED task і зчитує глобальний лічильник.
static void heartbeat_task(void* pvParameters) {
    (void)pvParameters;
    uint32_t seconds = 0;

    while (1) {
        if (xSemaphoreTake(g_press_semaphore, pdMS_TO_TICKS(HEARTBEAT_TASK_PERIOD_MS)) == pdTRUE) {
            uint32_t snapshot = g_press_counter;
            ESP_LOGI(TAG_HEARTBEAT, "signal=received, press_counter=%lu",
                     static_cast<unsigned long>(snapshot));
        } else {
            ESP_LOGI(TAG_HEARTBEAT, "waiting signal, uptime=%lu s",
                     static_cast<unsigned long>(seconds));
            seconds++;
        }
    }
}

extern "C" void app_main() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(gpio_set_level(LED_PIN, 0));

    g_press_semaphore = xSemaphoreCreateCounting(PRESS_SIGNAL_MAX_COUNT, 0);
    if (g_press_semaphore == nullptr) {
        ESP_LOGE(TAG_APP_MAIN, "Failed to create press semaphore");
        return;
    }

    gpio_config_t button_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&button_conf));

    TaskHandle_t heartbeat_handle = nullptr;

    BaseType_t rc_led = xTaskCreatePinnedToCore(led_task,
                                                 "led_task",
                                                 TASK_STACK_WORDS,
                                                 nullptr,
                                                 5,
                                                 nullptr,
                                                 LED_TASK_CORE);

    BaseType_t rc_heartbeat = xTaskCreatePinnedToCore(heartbeat_task,
                                                       "heartbeat_task",
                                                       TASK_STACK_WORDS,
                                                       nullptr,
                                                       1,
                                                       &heartbeat_handle,
                                                       HEARTBEAT_TASK_CORE);

    if (rc_led != pdPASS || rc_heartbeat != pdPASS) {
        ESP_LOGE(TAG_APP_MAIN, "Task creation failed: led=%ld heartbeat=%ld",
                 static_cast<long>(rc_led),
                 static_cast<long>(rc_heartbeat));
    }
}

