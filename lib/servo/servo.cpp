#include "servo.h"

static uint32_t servo_pulse_to_duty(const pwm_t *pwm, uint32_t pulse_width_us) {
	uint32_t period_us = 1000000 / pwm->config.frequency_hz;
	return (pwm->max_duty * pulse_width_us) / period_us;
}

esp_err_t servo_init(servo_t *servo, pwm_t *pwm) {
	if (!servo || !pwm || !pwm->initialized) {
		return ESP_ERR_INVALID_ARG;
	}

	esp_err_t ret = pwm_set_frequency(pwm, SERVO_FREQUENCY_HZ);
	if (ret != ESP_OK) {
		return ret;
	}

	*servo = {};
	servo->pwm = pwm;
	servo->initialized = true;
	return servo_set_angle(servo, 90);
}

esp_err_t servo_deinit(servo_t *servo) {
	if (!servo) {
		return ESP_ERR_INVALID_ARG;
	}
	if (!servo->initialized) {
		return ESP_OK;
	}

	*servo = {};
	return ESP_OK;
}

esp_err_t servo_set_angle(servo_t *servo, uint16_t angle) {
	if (!servo || !servo->initialized) {
		return ESP_ERR_INVALID_STATE;
	}
	if (angle > SERVO_MAX_ANGLE) {
		return ESP_ERR_INVALID_ARG;
	}

	uint32_t pulse_width_us = SERVO_MIN_PULSE_US +
		((SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) * angle) /
		(SERVO_MAX_ANGLE - SERVO_MIN_ANGLE);
	return servo_set_pulse_width(servo, pulse_width_us);
}

esp_err_t servo_set_pulse_width(servo_t *servo, uint32_t pulse_width_us) {
	if (!servo || !servo->initialized) {
		return ESP_ERR_INVALID_STATE;
	}
	if (pulse_width_us < SERVO_MIN_PULSE_US ||
		pulse_width_us > SERVO_MAX_PULSE_US) {
		return ESP_ERR_INVALID_ARG;
	}

	uint32_t duty = servo_pulse_to_duty(servo->pwm, pulse_width_us);
	esp_err_t ret = pwm_set_duty(servo->pwm, duty);
	if (ret == ESP_OK) {
		servo->pulse_width_us = pulse_width_us;
		servo->angle = SERVO_MIN_ANGLE +
			((pulse_width_us - SERVO_MIN_PULSE_US) *
			 (SERVO_MAX_ANGLE - SERVO_MIN_ANGLE)) /
			(SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US);
	}
	return ret;
}
