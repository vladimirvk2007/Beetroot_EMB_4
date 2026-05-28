#include <stdio.h>
#include "main.h"
#include "usbd_cdc_if.h"
#include "led.h"

extern "C" {
#include "FreeRTOS.h"
#include "task.h"
}

#define TASK_STACK_WORDS 256U

struct LedTaskConfig {
    const char* task_name;
    const Led* led;
    TickType_t period_ticks;
};

enum DemoStep : uint32_t {
    DEMO_STEP_GET_STATE = 0U,
    DEMO_STEP_GET_PRIORITY,
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

static void print_stack_usage(const char* name, TaskHandle_t handle, UBaseType_t stack_words) {
    if (handle == NULL) {
        printf("[STACK] %s: handle=NULL\r\n", name);
        return;
    }

    UBaseType_t hwm_words = uxTaskGetStackHighWaterMark(handle);
    UBaseType_t used_words = (stack_words > hwm_words) ? (stack_words - hwm_words) : 0U;

    printf("[STACK] %s used=%lu words, min-free=%lu words\r\n",
           name,
           (unsigned long)used_words,
           (unsigned long)hwm_words);
}

static void led_task(void* parameter) {
    const LedTaskConfig* cfg = static_cast<const LedTaskConfig*>(parameter);
    bool is_on = false;

    if ((cfg == NULL) || (cfg->led == NULL)) {
        printf("[LED] invalid task config\r\n");
        vTaskDelete(NULL);
    }

    for (;;) {
        if (ulTaskNotifyTake(pdTRUE, 0U) > 0U) {
            printf("[%s] notification received\r\n", cfg->task_name);
        }

        is_on = !is_on;
        if (is_on) {
            cfg->led->on();
            printf("[%s] ON\r\n", cfg->task_name);
        } else {
            cfg->led->off();
            printf("[%s] OFF\r\n", cfg->task_name);
        }

        vTaskDelay(cfg->period_ticks);
    }
}

static void heartbeat_task(void* parameter) {
    (void)parameter;
    uint32_t seconds = 0U;

    for (;;) {
        printf("[HEARTBEAT] app alive, uptime=%lu s\r\n", (unsigned long)seconds);
        seconds++;
        vTaskDelay(pdMS_TO_TICKS(1000U));
    }
}

extern "C" int _write(int file, char* ptr, int len) {
    (void)file;

    if ((ptr == NULL) || (len <= 0)) {
        return 0;
    }

    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        return len;
    }

    int sent = 0;
    while (sent < len) {
        uint16_t chunk = (uint16_t)((len - sent) > 64 ? 64 : (len - sent));
        uint8_t rc = CDC_Transmit_FS((uint8_t*)&ptr[sent], chunk);

        if (rc == USBD_OK) {
            sent += chunk;
            continue;
        }

        if (rc == USBD_BUSY) {
            vTaskDelay(pdMS_TO_TICKS(2U));
            continue;
        }

        break;
    }

    return sent;
}

static TaskHandle_t led_fast_handle = NULL;
static TaskHandle_t led_medium_handle = NULL;
static TaskHandle_t led_slow_handle = NULL;
static TaskHandle_t heartbeat_handle = NULL;

static void controller_task(void* parameter) {
    (void)parameter;
    uint32_t demo_step = DEMO_STEP_GET_STATE;

    for (;;) {
        TaskHandle_t* selected_handle = &led_medium_handle;
        eTaskState state;

        printf("[APP] handles fast=%p medium=%p slow=%p heartbeat=%p\r\n",
               (void*)led_fast_handle,
               (void*)led_medium_handle,
               (void*)led_slow_handle,
               (void*)heartbeat_handle);

        print_stack_usage("led_fast_task", led_fast_handle, TASK_STACK_WORDS);
        print_stack_usage("led_medium_task", led_medium_handle, TASK_STACK_WORDS);
        print_stack_usage("led_slow_task", led_slow_handle, TASK_STACK_WORDS);
        print_stack_usage("heartbeat_task", heartbeat_handle, TASK_STACK_WORDS);

        switch (demo_step) {
            case DEMO_STEP_GET_STATE:
                if (*selected_handle == NULL) {
                    break;
                }
                state = eTaskGetState(*selected_handle);
                printf("[APP] eTaskGetState(%s) -> %s\r\n",
                       pcTaskGetName(*selected_handle),
                       task_state_to_string(state));
                demo_step = DEMO_STEP_GET_PRIORITY;
                break;

            case DEMO_STEP_GET_PRIORITY:
                if (*selected_handle == NULL) {
                    break;
                }
                printf("[APP] uxTaskPriorityGet(%s) -> %lu\r\n",
                       pcTaskGetName(*selected_handle),
                       (unsigned long)uxTaskPriorityGet(*selected_handle));
                demo_step = DEMO_STEP_SUSPEND;
                break;

            case DEMO_STEP_SUSPEND:
                if (*selected_handle == NULL) {
                    break;
                }
                vTaskSuspend(*selected_handle);
                printf("[APP] vTaskSuspend(%s)\r\n", pcTaskGetName(*selected_handle));
                demo_step = DEMO_STEP_RESUME;
                break;

            case DEMO_STEP_RESUME:
                if (*selected_handle == NULL) {
                    break;
                }
                vTaskResume(*selected_handle);
                printf("[APP] vTaskResume(%s)\r\n", pcTaskGetName(*selected_handle));
                demo_step = DEMO_STEP_NOTIFY;
                break;

            case DEMO_STEP_NOTIFY:
                if (*selected_handle == NULL) {
                    break;
                }
                xTaskNotifyGive(*selected_handle);
                printf("[APP] xTaskNotifyGive(%s)\r\n", pcTaskGetName(*selected_handle));
                demo_step = DEMO_STEP_ABORT_DELAY;
                break;

            case DEMO_STEP_ABORT_DELAY:
                if (*selected_handle == NULL) {
                    break;
                }
#if defined(INCLUDE_xTaskAbortDelay) && (INCLUDE_xTaskAbortDelay == 1)
                printf("[APP] xTaskAbortDelay(%s) -> %ld\r\n",
                       pcTaskGetName(*selected_handle),
                       (long)xTaskAbortDelay(*selected_handle));
#else
                printf("[APP] xTaskAbortDelay(%s) is disabled in FreeRTOSConfig\r\n",
                       pcTaskGetName(*selected_handle));
#endif
                demo_step = DEMO_STEP_STACK_HIGH_WATER;
                break;

            case DEMO_STEP_STACK_HIGH_WATER:
                if (*selected_handle == NULL) {
                    break;
                }
                printf("[APP] uxTaskGetStackHighWaterMark(%s) -> %lu words\r\n",
                       pcTaskGetName(*selected_handle),
                       (unsigned long)uxTaskGetStackHighWaterMark(*selected_handle));
                demo_step = DEMO_STEP_DELETE;
                break;

            case DEMO_STEP_DELETE:
                if (*selected_handle == NULL) {
                    break;
                }
                printf("[APP] vTaskDelete(%s)\r\n", pcTaskGetName(*selected_handle));
                vTaskDelete(*selected_handle);
                *selected_handle = NULL;
                demo_step = DEMO_STEP_DONE;
                break;

            default:
                printf("[APP] demo complete\r\n");
                demo_step = DEMO_STEP_DONE;
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(5000U));
    }
}

extern "C" void main_cpp(void) {
    static bool started = false;

    static const Led led_fast(GPIOC, GPIO_PIN_13);
    static const Led led_medium(GPIOA, GPIO_PIN_0);
    static const Led led_slow(GPIOA, GPIO_PIN_1);

    static const LedTaskConfig led1_cfg = {"LED_FAST", &led_fast, pdMS_TO_TICKS(250U)};
    static const LedTaskConfig led2_cfg = {"LED_MEDIUM", &led_medium, pdMS_TO_TICKS(700U)};
    static const LedTaskConfig led3_cfg = {"LED_SLOW", &led_slow, pdMS_TO_TICKS(1300U)};

    if (started) {
        return;
    }
    started = true;

    BaseType_t rc_fast = xTaskCreate(led_task,
                                     "led_fast_task",
                                     TASK_STACK_WORDS,
                                     (void*)&led1_cfg,
                                     3U,
                                     &led_fast_handle);

    BaseType_t rc_medium = xTaskCreate(led_task,
                                       "led_medium_task",
                                       TASK_STACK_WORDS,
                                       (void*)&led2_cfg,
                                       3U,
                                       &led_medium_handle);

    BaseType_t rc_slow = xTaskCreate(led_task,
                                     "led_slow_task",
                                     TASK_STACK_WORDS,
                                     (void*)&led3_cfg,
                                     3U,
                                     &led_slow_handle);

    BaseType_t rc_heartbeat = xTaskCreate(heartbeat_task,
                                          "heartbeat_task",
                                          TASK_STACK_WORDS,
                                          NULL,
                                          1U,
                                          &heartbeat_handle);

    TaskHandle_t controller_handle = NULL;
    BaseType_t rc_controller = xTaskCreate(controller_task,
                                           "controller_task",
                                           TASK_STACK_WORDS + 128U,
                                           NULL,
                                           2U,
                                           &controller_handle);

    if ((rc_fast != pdPASS) ||
        (rc_medium != pdPASS) ||
        (rc_slow != pdPASS) ||
        (rc_heartbeat != pdPASS) ||
        (rc_controller != pdPASS)) {
        printf("[APP] task creation failed: fast=%ld medium=%ld slow=%ld heartbeat=%ld ctrl=%ld\r\n",
               (long)rc_fast,
               (long)rc_medium,
               (long)rc_slow,
               (long)rc_heartbeat,
               (long)rc_controller);
    }
}