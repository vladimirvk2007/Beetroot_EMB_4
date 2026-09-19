#ifndef LIB_SINE_SINE_H
#define LIB_SINE_SINE_H

#include <stdint.h>

#include "esp_err.h"
#include "esp_timer.h"
#include "pwm.h"

#define SINE_TABLE_SIZE 32

typedef struct {
	pwm_t *pwm;
	esp_timer_handle_t timer;
	uint32_t frequency_hz;
	uint32_t pwm_frequency_hz;
	uint32_t sample_period_us;
	uint8_t table_index;
	bool initialized;
	bool running;
} sine_t;

esp_err_t sine_init(sine_t *sine, pwm_t *pwm, uint32_t frequency_hz);
esp_err_t sine_deinit(sine_t *sine);
esp_err_t sine_start(sine_t *sine);
esp_err_t sine_stop(sine_t *sine);
esp_err_t sine_set_frequency(sine_t *sine, uint32_t frequency_hz);

#endif // LIB_SINE_SINE_H
