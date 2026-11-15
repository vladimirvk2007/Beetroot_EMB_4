#ifndef LIB_PWM_PWM_H
#define LIB_PWM_PWM_H

#include <stdbool.h>
#include <stdint.h>

#include "driver/ledc.h"
#include "esp_err.h"
#include "hal/gpio_types.h"

// Налаштування GPIO, каналу LEDC, частоти, роздільності та полярності PWM.
typedef struct {
	gpio_num_t gpio;
	ledc_channel_t channel;
	ledc_timer_t timer;
	uint32_t frequency_hz;
	ledc_timer_bit_t resolution;
	uint32_t duty;
	bool inverted;
} pwm_config_t;

// Стан PWM-каналу та його максимальне значення duty.
typedef struct {
	pwm_config_t config;
	uint32_t max_duty;
	bool initialized;
	bool running;
} pwm_t;

#ifdef __cplusplus
extern "C" {
#endif

// Налаштовує таймер LEDC і запускає PWM на заданому GPIO.
esp_err_t pwm_init(pwm_t *pwm, const pwm_config_t *config);

// Зупиняє PWM, скидає GPIO та очищує стан каналу.
esp_err_t pwm_deinit(pwm_t *pwm);

// Оновлює й запускає PWM із раніше встановленою шпаруватістю.
esp_err_t pwm_start(pwm_t *pwm);

// Зупиняє PWM і встановлює вихідний рівень GPIO.
esp_err_t pwm_stop(pwm_t *pwm);

// Встановлює шпаруватість у відліках LEDC, від 0 до max_duty.
esp_err_t pwm_set_duty(pwm_t *pwm, uint32_t duty);

// Встановлює шпаруватість у відсотках, від 0 до 100.
esp_err_t pwm_set_percent(pwm_t *pwm, uint8_t percent);

#ifdef __cplusplus
}
#endif

#endif // LIB_PWM_PWM_H
