#include "servo/servo.h"

#define SERVO_PERIOD_US 20000

static uint32_t Servo_GetPulseUs(uint16_t angle) {
	if (angle > SERVO_MAX_ANGLE) {
		angle = SERVO_MAX_ANGLE;
	}

	return SERVO_MIN_PULSE_US +
		   ((SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) * angle) /
			   SERVO_MAX_ANGLE;
}

bool Servo_Init(Servo_t *servo, PwmDriver_t *pwm) {
	if (servo == NULL || pwm == NULL || !pwm->initialized || !pwm->running) {
		return false;
	}

	servo->pwm = pwm;
	servo->angle = 0;
	servo->initialized = false;

	Pwm_SetFrequency(pwm, SERVO_FREQUENCY_HZ);
	servo->initialized = true;
	Servo_SetAngle(servo, 0);
	return true;
}

void Servo_SetAngle(Servo_t *servo, uint16_t angle) {
	if (servo == NULL || !servo->initialized || servo->pwm == NULL) {
		return;
	}

	if (angle > SERVO_MAX_ANGLE) {
		angle = SERVO_MAX_ANGLE;
	}

	Servo_SetPulseWidth(servo, Servo_GetPulseUs(angle));
	servo->angle = angle;
}

void Servo_SetPulseWidth(Servo_t *servo, uint32_t pulse_width_us) {
	if (servo == NULL || !servo->initialized || servo->pwm == NULL) {
		return;
	}

	if (pulse_width_us < SERVO_MIN_PULSE_US) {
		pulse_width_us = SERVO_MIN_PULSE_US;
	} else if (pulse_width_us > SERVO_MAX_PULSE_US) {
		pulse_width_us = SERVO_MAX_PULSE_US;
	}

	uint32_t compare_value =
		((servo->pwm->period + 1) * pulse_width_us) / SERVO_PERIOD_US;
	Pwm_SetDutyCycle(servo->pwm, compare_value);
}

void Servo_Deinit(Servo_t *servo) {
	if (servo == NULL) {
		return;
	}

	servo->pwm = NULL;
	servo->angle = 0;
	servo->initialized = false;
}
