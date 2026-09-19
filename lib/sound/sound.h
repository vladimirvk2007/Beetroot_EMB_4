#ifndef LIB_SOUND_SOUND_H
#define LIB_SOUND_SOUND_H

#include <stdint.h>

#include "esp_err.h"
#include "esp_timer.h"
#include "pwm.h"

#define SOUND_SINE_TABLE_SIZE 32

typedef struct {
	pwm_t *pwm;
	esp_timer_handle_t timer;
	uint32_t frequency_hz;
	uint32_t pwm_frequency_hz;
	uint32_t sample_period_us;
	uint8_t table_index;
	bool initialized;
	bool running;
} sound_t;

esp_err_t sound_init(sound_t *sound, pwm_t *pwm, uint32_t frequency_hz);
esp_err_t sound_deinit(sound_t *sound);
esp_err_t sound_start(sound_t *sound);
esp_err_t sound_stop(sound_t *sound);
esp_err_t sound_set_frequency(sound_t *sound, uint32_t frequency_hz);

#endif // LIB_SOUND_SOUND_H
