#ifndef SOUND_H
#define SOUND_H

#include "pwm/pwm.h"

#include <stdbool.h>
#include <stdint.h>

bool Sound_Init(PwmDriver_t *pwm, uint32_t frequency_hz);
uint32_t Sound_GetPwmFrequency(uint32_t frequency_hz);
void Sound_Stop(void);

#endif
