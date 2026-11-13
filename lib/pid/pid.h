#ifndef LIB_PID_PID_H
#define LIB_PID_PID_H

#include <stdbool.h>

#include "esp_err.h"

// Параметри PID-регулятора та обмеження його складових.
typedef struct {
    float kp;
    float ki;
    float kd;
    float output_min;
    float output_max;
    float integral_min;
    float integral_max;
} pid_config_t;

// Внутрішній стан PID-регулятора.
typedef struct {
    pid_config_t config;
    float integral;
    float previous_measurement;
    bool has_previous_measurement;
    bool initialized;
} pid_controller_t;

#ifdef __cplusplus
extern "C" {
#endif

// Ініціалізує PID-регулятор і перевіряє коректність параметрів.
esp_err_t pid_init(pid_controller_t *pid, const pid_config_t *config);

// Обчислює вихід PID для заданого значення, вимірювання та інтервалу часу в секундах.
esp_err_t pid_compute(pid_controller_t *pid,
                      float setpoint,
                      float measurement,
                      float dt_seconds,
                      float *output);

// Скидає інтегральну та диференціальну історію регулятора.
esp_err_t pid_reset(pid_controller_t *pid);

#ifdef __cplusplus
}
#endif

#endif // LIB_PID_PID_H
