#ifndef LIB_PWM_PWM_H
#define LIB_PWM_PWM_H

#include <stdbool.h>
#include <stdint.h>

#include "driver/ledc.h"
#include "esp_err.h"
#include "hal/gpio_types.h"

typedef struct {
	gpio_num_t gpio;
	ledc_channel_t channel;
	ledc_timer_t timer;
	uint32_t frequency_hz;
	ledc_timer_bit_t resolution;
	uint32_t duty;
	bool inverted;
} pwm_config_t;

typedef struct {
	pwm_config_t config;
	uint32_t max_duty;
	bool initialized;
	bool running;
} pwm_t;

esp_err_t pwm_init(pwm_t *pwm, const pwm_config_t *config);
esp_err_t pwm_deinit(pwm_t *pwm);
esp_err_t pwm_start(pwm_t *pwm);
esp_err_t pwm_stop(pwm_t *pwm);
esp_err_t pwm_set_frequency(pwm_t *pwm, uint32_t frequency_hz);
esp_err_t pwm_set_duty(pwm_t *pwm, uint32_t duty);
esp_err_t pwm_set_percent(pwm_t *pwm, uint8_t percent);

#endif // LIB_PWM_PWM_H
