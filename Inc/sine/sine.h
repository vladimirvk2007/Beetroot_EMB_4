#ifndef SINE_H
#define SINE_H

#include "pwm/pwm.h"

#include <stdbool.h>
#include <stdint.h>

bool Sine_Init(PwmDriver_t *pwm, uint32_t frequency_hz);
uint32_t Sine_GetPwmFrequency(uint32_t frequency_hz);
void Sine_Stop(void);

#endif
