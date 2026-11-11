#include "drv8833.h"

#include <string.h>

// Ініціалізує PWM-канали двигуна з нульовою шпаруватістю.
esp_err_t drv8833_init(drv8833_t *motor, const drv8833_config_t *config) {
    if (motor == NULL || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (config->in1.gpio == config->in2.gpio ||
        config->in1.channel == config->in2.channel ||
        config->in1.timer != config->in2.timer ||
        config->in1.frequency_hz != config->in2.frequency_hz ||
        config->in1.resolution != config->in2.resolution) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(motor, 0, sizeof(*motor));

    pwm_config_t in1_config = config->in1;
    pwm_config_t in2_config = config->in2;
    in1_config.duty = 0;
    in2_config.duty = 0;

    esp_err_t err = pwm_init(&motor->in1_pwm, &in1_config);
    if (err != ESP_OK) {
        return err;
    }

    err = pwm_init(&motor->in2_pwm, &in2_config);
    if (err != ESP_OK) {
        esp_err_t cleanup_err = pwm_deinit(&motor->in1_pwm);
        return cleanup_err == ESP_OK ? err : cleanup_err;
    }

    motor->initialized = true;
    return ESP_OK;
}

// Зупиняє PWM-канали та звільняє ресурси двигуна.
esp_err_t drv8833_deinit(drv8833_t *motor) {
    if (motor == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!motor->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t in1_err = pwm_deinit(&motor->in1_pwm);
    esp_err_t in2_err = pwm_deinit(&motor->in2_pwm);
    motor->initialized = false;
    motor->speed_percent = 0;

    return in1_err != ESP_OK ? in1_err : in2_err;
}

// Встановлює напрямок і швидкість у межах від -100% до +100%.
esp_err_t drv8833_set_speed(drv8833_t *motor, int16_t speed_percent) {
    if (motor == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!motor->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (speed_percent < -100 || speed_percent > 100) {
        return ESP_ERR_INVALID_ARG;
    }

    if (speed_percent == 0) {
        esp_err_t err = pwm_set_percent(&motor->in1_pwm, 0);
        if (err != ESP_OK) {
            return err;
        }
        err = pwm_set_percent(&motor->in2_pwm, 0);
        if (err == ESP_OK) {
            motor->speed_percent = 0;
        }
        return err;
    }

    pwm_t *active_pwm = speed_percent > 0 ? &motor->in1_pwm : &motor->in2_pwm;
    pwm_t *inactive_pwm = speed_percent > 0 ? &motor->in2_pwm : &motor->in1_pwm;

    esp_err_t err = pwm_set_percent(inactive_pwm, 0);
    if (err != ESP_OK) {
        return err;
    }

    if ((motor->speed_percent > 0 && speed_percent < 0) ||
        (motor->speed_percent < 0 && speed_percent > 0)) {
        motor->speed_percent = 0;
    }

    err = pwm_set_percent(active_pwm, (uint8_t)(speed_percent > 0
                                                     ? speed_percent
                                                     : -speed_percent));
    if (err == ESP_OK) {
        motor->speed_percent = speed_percent;
    }
    return err;
}
