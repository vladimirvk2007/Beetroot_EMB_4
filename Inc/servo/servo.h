#ifndef SERVO_H
#define SERVO_H

#include "pwm/pwm.h"

#include <stdbool.h>
#include <stdint.h>

#define SERVO_MIN_ANGLE 0
#define SERVO_MAX_ANGLE 180
#define SERVO_FREQUENCY_HZ 50
#define SERVO_MIN_PULSE_US 500
#define SERVO_MAX_PULSE_US 2400

typedef struct {
	PwmDriver_t *pwm;
	uint16_t angle;
	bool initialized;
} Servo_t;

bool Servo_Init(Servo_t *servo, PwmDriver_t *pwm);
void Servo_SetAngle(Servo_t *servo, uint16_t angle);
void Servo_SetPulseWidth(Servo_t *servo, uint32_t pulse_width_us);
void Servo_Deinit(Servo_t *servo);

#endif
