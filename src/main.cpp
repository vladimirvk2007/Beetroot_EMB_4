#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_1_PIN GPIO_NUM_16
#define LED_2_PIN GPIO_NUM_17
#define LED_3_PIN GPIO_NUM_18

#define TASK_STACK_WORDS 2048
#define LED_TASK_CORE 1
#define HEARTBEAT_TASK_CORE 0

static const char* TAG_LED_TASK = "LED_TASK";
static const char* TAG_HEARTBEAT = "HEARTBEAT";
static const char* TAG_STACK = "STACK";
static const char* TAG_APP_MAIN = "APP_MAIN";

class Led {
public:
    Led(gpio_num_t pin) : pin_(pin) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << pin_),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_conf);
    }
    void on() const {
        gpio_set_level(pin_, 0);
    }
    void off() const {
        gpio_set_level(pin_, 1);
    }
private:
    const gpio_num_t pin_;
};

struct LedTaskConfig {
    gpio_num_t pin;
    TickType_t period;
    const char* taskName;
};

// Кроки демонстрації керування таском через handle.
enum DemoStep : uint32_t {
    DEMO_STEP_GET_STATE = 0,
    DEMO_STEP_SET_PRIORITY,
    DEMO_STEP_SUSPEND,
    DEMO_STEP_RESUME,
    DEMO_STEP_NOTIFY,
    DEMO_STEP_ABORT_DELAY,
    DEMO_STEP_STACK_HIGH_WATER,
    DEMO_STEP_DELETE,
    DEMO_STEP_DONE
};

static const char* task_state_to_string(eTaskState state) {
    switch (state) {
        case eRunning:
            return "Running";
        case eReady:
            return "Ready";
        case eBlocked:
            return "Blocked";
        case eSuspended:
            return "Suspended";
        case eDeleted:
            return "Deleted";
        default:
            return "Invalid";
    }
}

// Таск, який періодично перемикає LED та обробляє notification.
static void led_task(void* pvParameters) {
    if (pvParameters == nullptr) {
        ESP_LOGE(TAG_LED_TASK, "invalid config: pvParameters is null, deleting task");
        vTaskDelete(nullptr);
        return;
    }

    const LedTaskConfig* cfg = static_cast<const LedTaskConfig*>(pvParameters);
    Led led(cfg->pin);
    bool ledOn = false;

    while (1) {
        // Якщо app_main надіслав notification — виводимо подію в лог.
        if (ulTaskNotifyTake(pdTRUE, 0) > 0) {
            ESP_LOGI(cfg->taskName, "notification received");
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

// Фоновий heartbeat таск для індикації, що система жива.
static void heartbeat_task(void* pvParameters) {
    (void)pvParameters;
    uint32_t seconds = 0;
    while (1) {
        ESP_LOGI(TAG_HEARTBEAT, "app alive, uptime: %lu s", static_cast<unsigned long>(seconds));
        seconds++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
// Функція для виведення інформації про використання стеку таском.
static void print_stack_usage(const char* name, TaskHandle_t handle, UBaseType_t stackWords) {
    if (handle == nullptr) {
        ESP_LOGW(TAG_STACK, "%s: handle=null", name);
        return;
    }

    UBaseType_t hwmWords = uxTaskGetStackHighWaterMark(handle);
    UBaseType_t usedWords = 0;
    if (stackWords > hwmWords) {
        usedWords = stackWords - hwmWords;
    } else {
        usedWords = 0;
    }
    size_t usedBytes = usedWords * sizeof(StackType_t);
    size_t hwmBytes = hwmWords * sizeof(StackType_t);

    ESP_LOGI(TAG_STACK,
             "%s used=%lu words (%lu B), min-free=%lu words (%lu B)",
             name,
             static_cast<unsigned long>(usedWords),
             static_cast<unsigned long>(usedBytes),
             static_cast<unsigned long>(hwmWords),
             static_cast<unsigned long>(hwmBytes));
}

extern "C" void app_main() {
    static constexpr UBaseType_t LED_TASK_STACK_WORDS = TASK_STACK_WORDS;
    static constexpr UBaseType_t HEARTBEAT_STACK_WORDS = TASK_STACK_WORDS;

    // Налаштування рівнів логування по TAG на старті програми.
    //esp_log_level_set(TAG_APP_MAIN, ESP_LOG_INFO);
    //esp_log_level_set(TAG_LED_TASK, ESP_LOG_INFO);
    //esp_log_level_set(TAG_HEARTBEAT, ESP_LOG_INFO);
    //esp_log_level_set(TAG_STACK, ESP_LOG_INFO);

    esp_log_level_set("*", ESP_LOG_NONE);          // вимкнути всі TAG-и
    esp_log_level_set(TAG_APP_MAIN, ESP_LOG_INFO); // увімкнути тільки APP_MAIN

    static const LedTaskConfig led1 = {LED_1_PIN, pdMS_TO_TICKS(250), "LED_FAST"};
    static const LedTaskConfig led2 = {LED_2_PIN, pdMS_TO_TICKS(700), "LED_MEDIUM"};
    static const LedTaskConfig led3 = {LED_3_PIN, pdMS_TO_TICKS(1300), "LED_SLOW"};

    TaskHandle_t ledFastHandle = nullptr;
    TaskHandle_t ledMediumHandle = nullptr;
    TaskHandle_t ledSlowHandle = nullptr;
    TaskHandle_t heartbeatHandle = nullptr;

    // Створення тасків та збереження їх handle.
    BaseType_t rcFast = xTaskCreatePinnedToCore(led_task,
                                                "led_fast_task", // ім'я таска
                                                LED_TASK_STACK_WORDS, // розмір стеку (слова)
                                                (void*)&led1,    // параметри таска
                                                5,               // пріоритет таска (0..configMAX_PRIORITIES-1; тут 5)
                                                &ledFastHandle,  // таск handle
                                                LED_TASK_CORE);  // ядро
    BaseType_t rcMedium = xTaskCreatePinnedToCore(led_task,
                                                   "led_medium_task",
                                                   LED_TASK_STACK_WORDS,
                                                   (void*)&led2,
                                                   5,
                                                   &ledMediumHandle,
                                                   LED_TASK_CORE);
    BaseType_t rcSlow = xTaskCreatePinnedToCore(led_task,
                                                 "led_slow_task",
                                                 LED_TASK_STACK_WORDS,
                                                 (void*)&led3,
                                                 5,
                                                 &ledSlowHandle,
                                                 LED_TASK_CORE);
    BaseType_t rcHeartbeat = xTaskCreatePinnedToCore(heartbeat_task,
                                                      "heartbeat_task",
                                                      HEARTBEAT_STACK_WORDS,
                                                      nullptr,
                                                      1,
                                                      &heartbeatHandle,
                                                      HEARTBEAT_TASK_CORE);

    if ((rcFast != pdPASS) || (rcMedium != pdPASS) || (rcSlow != pdPASS) || (rcHeartbeat != pdPASS)) {
        ESP_LOGE(TAG_APP_MAIN,
                 "task creation failed: fast=%ld medium=%ld slow=%ld heartbeat=%ld",
                 (long)rcFast,
                 (long)rcMedium,
                 (long)rcSlow,
                 (long)rcHeartbeat);
    }

    while (1) {
        // Усі дії в switch виконуються над одним вибраним таском.
        static uint32_t demoStep = DEMO_STEP_GET_STATE;
        TaskHandle_t* selectedHandle = &ledMediumHandle;
        eTaskState state = eDeleted;
        UBaseType_t oldPriority = 0;
        UBaseType_t newPriority = 0;
        BaseType_t aborted = pdFAIL;
        UBaseType_t highWaterMark = 0;

        ESP_LOGI(TAG_APP_MAIN,
             "handles fast=%p medium=%p slow=%p heartbeat=%p",
             (void*)ledFastHandle,
             (void*)ledMediumHandle,
             (void*)ledSlowHandle,
             (void*)heartbeatHandle);

        // Вимірювання фактичного використання стеку для кожного таска.
        print_stack_usage("led_fast_task", ledFastHandle, LED_TASK_STACK_WORDS);
        print_stack_usage("led_medium_task", ledMediumHandle, LED_TASK_STACK_WORDS);
        print_stack_usage("led_slow_task", ledSlowHandle, LED_TASK_STACK_WORDS);
        print_stack_usage("heartbeat_task", heartbeatHandle, HEARTBEAT_STACK_WORDS);

        switch (demoStep) {
            case DEMO_STEP_GET_STATE:
                // 1) Читаємо стан таска.
                if (*selectedHandle == nullptr) {
                    break;
                }
                state = eTaskGetState(*selectedHandle);
                ESP_LOGI(TAG_APP_MAIN, "eTaskGetState(selected_task) -> %s", task_state_to_string(state));
                demoStep = DEMO_STEP_SET_PRIORITY;
                break;

            case DEMO_STEP_SET_PRIORITY:
                // 2) Отримуємо та змінюємо пріоритет.
                if (*selectedHandle == nullptr) {
                    break;
                }
                oldPriority = uxTaskPriorityGet(*selectedHandle);
                if (oldPriority > 1U) {
                    newPriority = oldPriority - 1U;
                } else {
                    newPriority = oldPriority + 1U;
                }
                vTaskPrioritySet(*selectedHandle, newPriority);
                  ESP_LOGI(TAG_APP_MAIN,
                        "priority selected_task %lu -> %lu",
                        (unsigned long)oldPriority,
                        (unsigned long)uxTaskPriorityGet(*selectedHandle));
                demoStep = DEMO_STEP_SUSPEND;
                break;

            case DEMO_STEP_SUSPEND:
                // 3) Призупиняємо таск.
                if (*selectedHandle == nullptr) {
                    break;
                }
                vTaskSuspend(*selectedHandle);
                ESP_LOGW(TAG_APP_MAIN, "vTaskSuspend(selected_task)");
                demoStep = DEMO_STEP_RESUME;
                break;

            case DEMO_STEP_RESUME:
                // 4) Відновлюємо таск.
                if (*selectedHandle == nullptr) {
                    break;
                }
                vTaskResume(*selectedHandle);
                ESP_LOGW(TAG_APP_MAIN, "vTaskResume(selected_task)");
                demoStep = DEMO_STEP_NOTIFY;
                break;

            case DEMO_STEP_NOTIFY:
                // 5) Надсилаємо notification у таск.
                if (*selectedHandle == nullptr) {
                    break;
                }
                xTaskNotifyGive(*selectedHandle);
                ESP_LOGI(TAG_APP_MAIN, "xTaskNotifyGive(selected_task)");
                demoStep = DEMO_STEP_ABORT_DELAY;
                break;

            case DEMO_STEP_ABORT_DELAY:
                // 6) Перериваємо vTaskDelay, якщо таск заблокований у delay.
                if (*selectedHandle == nullptr) {
                    break;
                }
                aborted = xTaskAbortDelay(*selectedHandle);
                ESP_LOGI(TAG_APP_MAIN, "xTaskAbortDelay(selected_task) -> %ld", (long)aborted);
                demoStep = DEMO_STEP_STACK_HIGH_WATER;
                break;

            case DEMO_STEP_STACK_HIGH_WATER:
                // 7) Перевіряємо мінімальний запас стеку.
                if (*selectedHandle == nullptr) {
                    break;
                }
                highWaterMark = uxTaskGetStackHighWaterMark(*selectedHandle);
                ESP_LOGI(TAG_APP_MAIN,
                         "uxTaskGetStackHighWaterMark(selected_task) -> %lu words",
                         (unsigned long)highWaterMark);
                demoStep = DEMO_STEP_DELETE;
                break;

            case DEMO_STEP_DELETE:
                // 8) Видаляємо таск через handle.
                if (*selectedHandle == nullptr) {
                    break;
                }
                vTaskDelete(*selectedHandle);
                *selectedHandle = nullptr;
                ESP_LOGE(TAG_APP_MAIN, "vTaskDelete(selected_task)");
                demoStep = DEMO_STEP_DONE;
                break;

            default:
                ESP_LOGI(TAG_APP_MAIN, "demo complete, monitoring handles only");
                demoStep = DEMO_STEP_DONE;
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

