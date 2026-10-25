#ifndef LIB_SERVO_SERVO_H
#define LIB_SERVO_SERVO_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "pwm.h"

#define SERVO_FREQUENCY_HZ 50
#define SERVO_MIN_ANGLE 0
#define SERVO_MAX_ANGLE 180
#define SERVO_MIN_PULSE_US 500
#define SERVO_MAX_PULSE_US 2500

typedef struct {
	pwm_t *pwm;
	uint16_t angle;
	uint32_t pulse_width_us;
	bool initialized;
} servo_t;

esp_err_t servo_init(servo_t *servo, pwm_t *pwm);
esp_err_t servo_deinit(servo_t *servo);
esp_err_t servo_set_angle(servo_t *servo, uint16_t angle);
esp_err_t servo_set_pulse_width(servo_t *servo, uint32_t pulse_width_us);

#endif // LIB_SERVO_SERVO_H
