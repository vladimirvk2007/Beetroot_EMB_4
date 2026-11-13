#include "pid.h"

#include <math.h>
#include <string.h>

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
           config->integral_min <= config->integral_max;
}

esp_err_t pid_init(pid_controller_t *pid, const pid_config_t *config) {
    if (pid == NULL || !pid_config_valid(config)) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(pid, 0, sizeof(*pid));
    pid->config = *config;
    pid->initialized = true;
    return ESP_OK;
}

esp_err_t pid_compute(pid_controller_t *pid,
                      float setpoint,
                      float measurement,
                      float dt_seconds,
                      float *output) {
    if (pid == NULL || output == NULL ||
        !isfinite(setpoint) || !isfinite(measurement) ||
        !isfinite(dt_seconds) || dt_seconds <= 0.0f) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!pid->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    const float error = setpoint - measurement;
    const float proportional = pid->config.kp * error;
    const float derivative = pid->has_previous_measurement
                                 ? -pid->config.kd *
                                       (measurement - pid->previous_measurement) /
                                       dt_seconds
                                 : 0.0f;

    float candidate_integral = pid->integral + error * dt_seconds;
    if (candidate_integral > pid->config.integral_max) {
        candidate_integral = pid->config.integral_max;
    } else if (candidate_integral < pid->config.integral_min) {
        candidate_integral = pid->config.integral_min;
    }

    const float candidate_output = proportional +
                                  pid->config.ki * candidate_integral +
                                  derivative;
    const bool saturating_high = candidate_output > pid->config.output_max &&
                                 error > 0.0f;
    const bool saturating_low = candidate_output < pid->config.output_min &&
                                error < 0.0f;
    if (!saturating_high && !saturating_low) {
        pid->integral = candidate_integral;
    }

    float result = proportional + pid->config.ki * pid->integral + derivative;
    if (result > pid->config.output_max) {
        result = pid->config.output_max;
    } else if (result < pid->config.output_min) {
        result = pid->config.output_min;
    }

    pid->previous_measurement = measurement;
    pid->has_previous_measurement = true;
    *output = result;
    return ESP_OK;
}

esp_err_t pid_reset(pid_controller_t *pid) {
    if (pid == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!pid->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    pid->integral = 0.0f;
    pid->previous_measurement = 0.0f;
    pid->has_previous_measurement = false;
    return ESP_OK;
}
