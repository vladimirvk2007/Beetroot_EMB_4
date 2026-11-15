#include "pid.h"

#include <math.h>
#include <string.h>

// Перевіряє скінченність коефіцієнтів і коректність меж регулятора.
static bool pid_config_valid(const pid_config_t *config) {
    return config != NULL &&
           isfinite(config->kp) &&
           isfinite(config->ki) &&
           isfinite(config->kd) &&
           isfinite(config->output_min) &&
           isfinite(config->output_max) &&
           isfinite(config->integral_min) &&
           isfinite(config->integral_max) &&
           config->output_min < config->output_max &&
           config->output_min >= -100.0f &&
           config->output_max <= 100.0f &&
           config->integral_min <= config->integral_max;
}

esp_err_t pid_init(pid_controller_t *pid, const pid_config_t *config) {
    if (pid == NULL || !pid_config_valid(config)) {
        return ESP_ERR_INVALID_ARG;
    }

    // Починає роботу з нульового стану та зберігає налаштування.
    memset(pid, 0, sizeof(*pid));
    pid->config = *config;
    pid->initialized = true;
    return ESP_OK;
}

esp_err_t pid_compute(pid_controller_t *pid,
                      float setpoint,
                      float measurement,
                      uint32_t dt_ms,
                      int8_t *output_percent) {
    if (pid == NULL || output_percent == NULL ||
        !isfinite(setpoint) || !isfinite(measurement) ||
        dt_ms == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!pid->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    // Переводить інтервал у секунди для інтегральної та диференціальної складових.
    const float dt_seconds = (float)dt_ms / 1000.0f;
    const float error = setpoint - measurement;
    const float proportional = pid->config.kp * error;
    // Похідна за вимірюванням зменшує стрибок виходу при зміні цілі.
    const float derivative = pid->has_previous_measurement
                                 ? -pid->config.kd *
                                       (measurement - pid->previous_measurement) /
                                       dt_seconds
                                 : 0.0f;

    // Обмежує накопичений інтеграл, щоб він не зростав безмежно.
    float candidate_integral = pid->integral + error * dt_seconds;
    if (candidate_integral > pid->config.integral_max) {
        candidate_integral = pid->config.integral_max;
    } else if (candidate_integral < pid->config.integral_min) {
        candidate_integral = pid->config.integral_min;
    }

    const float candidate_output = proportional +
                                  pid->config.ki * candidate_integral +
                                  derivative;
    // Не накопичує інтеграл, якщо він ще сильніше насичував би вихід.
    const bool saturating_high = candidate_output > pid->config.output_max &&
                                 error > 0.0f;
    const bool saturating_low = candidate_output < pid->config.output_min &&
                                error < 0.0f;
    if (!saturating_high && !saturating_low) {
        pid->integral = candidate_integral;
    }

    // Обмежує результат налаштованим діапазоном і переводить його у відсотки.
    float result = proportional + pid->config.ki * pid->integral + derivative;
    if (result > pid->config.output_max) {
        result = pid->config.output_max;
    } else if (result < pid->config.output_min) {
        result = pid->config.output_min;
    }

    pid->previous_measurement = measurement;
    pid->has_previous_measurement = true;
    *output_percent = (int8_t)lroundf(result);
    return ESP_OK;
}

esp_err_t pid_reset(pid_controller_t *pid) {
    if (pid == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!pid->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    // Очищає інтеграл і попереднє вимірювання перед новою ціллю.
    pid->integral = 0.0f;
    pid->previous_measurement = 0.0f;
    pid->has_previous_measurement = false;
    return ESP_OK;
}
