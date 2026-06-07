#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led.h"

#define LED_1_PIN GPIO_NUM_16

#define TASK_STACK_WORDS 2048
#define LED_TASK_CORE 1
#define HEARTBEAT_TASK_CORE 0
#define QUEUE_LENGTH 11
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define HEARTBEAT_TASK_PERIOD_MS 1000
#define LED_TASK_PERIOD_MS 100

static const char* TAG_LED_TASK = "LED_TASK";
static const char* TAG_HEARTBEAT = "HEARTBEAT";
static const char* TAG_STACK = "STACK";
static const char* TAG_APP_MAIN = "APP_MAIN";

struct LedTaskConfig {
    gpio_num_t pin;
    TickType_t period;
    const char* taskName;
};


// Черга для передачі uint64_t
static QueueHandle_t g_queue = nullptr;

// Таск, який періодично перемикає LED та надсилає елементи масиву в чергу.
static void led_task(void* pvParameters) {
    if (pvParameters == nullptr) {
        ESP_LOGE(TAG_LED_TASK, "Invalid config: pvParameters is null, deleting task");
        vTaskDelete(nullptr);
        return;
    }

    const LedTaskConfig* cfg = static_cast<const LedTaskConfig*>(pvParameters);
    Led led(cfg->pin);
    bool ledOn = false;

    // Приклад масиву для передачі
    static const uint64_t data_array[] = {10, 20, 30, 40, 50};
    size_t idx = 0;

    while (1) {
        // Надсилаємо черговий елемент масиву в чергу
        if (g_queue) {
            uint64_t value = data_array[idx];
            BaseType_t rc = xQueueSend(g_queue, &value, pdMS_TO_TICKS(100));
            if (rc == pdPASS) {
                ESP_LOGI(cfg->taskName, "Sent value %llu to queue", value);
            } else {
                ESP_LOGW(cfg->taskName, "Queue full, value %llu not sent", value);
            }
            idx = (idx + 1) % ARRAY_SIZE(data_array);
        }

        ledOn = !ledOn;
        if (ledOn) {
            led.on();
            ESP_LOGI(cfg->taskName, "LED GPIO %d -> ON", cfg->pin);
        } else {
            led.off();
            ESP_LOGI(cfg->taskName, "LED GPIO %d -> OFF", cfg->pin);
        }
        vTaskDelay(cfg->period);
    }
}


// Heartbeat таск приймає uint64_t з черги і логує їх
static void heartbeat_task(void* pvParameters) {
    (void)pvParameters;
    uint32_t seconds = 0;
    uint64_t rx_value = 0;
    while (1) {
        // Пробуємо отримати значення з черги (очікування 500 мс)
        if (g_queue) {
            BaseType_t rc = xQueueReceive(g_queue, &rx_value, pdMS_TO_TICKS(500));
            if (rc != pdPASS) {
                ESP_LOGI(TAG_HEARTBEAT, "Queue empty, value not received");
            } else {
                while (rc == pdPASS) {
                    ESP_LOGI(TAG_HEARTBEAT, "Received from queue: %llu", rx_value);
                    rc = xQueueReceive(g_queue, &rx_value, pdMS_TO_TICKS(500));
                }
            }
        }
        ESP_LOGI(TAG_HEARTBEAT, "app alive, uptime: %lu s", static_cast<unsigned long>(seconds));
        seconds++;
        vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_TASK_PERIOD_MS));
    }
}

extern "C" void app_main() {
    static constexpr UBaseType_t LED_TASK_STACK_WORDS = TASK_STACK_WORDS;
    static constexpr UBaseType_t HEARTBEAT_STACK_WORDS = TASK_STACK_WORDS;

    // Налаштування рівнів логування по TAG на старті програми.
    esp_log_level_set(TAG_APP_MAIN, ESP_LOG_INFO);
    esp_log_level_set(TAG_LED_TASK, ESP_LOG_INFO);
    esp_log_level_set(TAG_HEARTBEAT, ESP_LOG_INFO);
    esp_log_level_set(TAG_STACK, ESP_LOG_INFO);


    static const LedTaskConfig led1 = {LED_1_PIN, pdMS_TO_TICKS(LED_TASK_PERIOD_MS),
                                        "LED_FAST"};

    TaskHandle_t ledFastHandle = nullptr;
    TaskHandle_t heartbeatHandle = nullptr;


    // Створюємо чергу для передачі uint64_t
    g_queue = xQueueCreate(QUEUE_LENGTH, sizeof(uint64_t));
    if (!g_queue) {
        ESP_LOGE(TAG_APP_MAIN, "Failed to create queue");
    }

    // Створення тасків та збереження їх handle.
    BaseType_t rcFast = xTaskCreatePinnedToCore(led_task,
                                                "led_fast_task", // ім'я таска
                                                LED_TASK_STACK_WORDS, // розмір стеку (слова)
                                                (void*)&led1,    // параметри таска
                                                5,               // пріоритет таска (0..configMAX_PRIORITIES-1; тут 5)
                                                &ledFastHandle,  // таск handle
                                                LED_TASK_CORE);  // ядро

    BaseType_t rcHeartbeat = xTaskCreatePinnedToCore(heartbeat_task,
                                                     "heartbeat_task",
                                                     HEARTBEAT_STACK_WORDS,
                                                     nullptr,
                                                     1,
                                                     &heartbeatHandle,
                                                     HEARTBEAT_TASK_CORE);

    if (rcFast != pdPASS || rcHeartbeat != pdPASS) {
        ESP_LOGE(TAG_APP_MAIN,
                 "Task creation failed: fast=%ld heartbeat=%ld",
                 (long)rcFast,
                 (long)rcHeartbeat);
    }
}
