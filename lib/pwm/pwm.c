#include <string.h>

#include "driver/gpio.h"
#include "pwm.h"

static const ledc_mode_t PWM_SPEED_MODE = LEDC_LOW_SPEED_MODE;

// Перевіряє GPIO та допустимість параметрів LEDC до конфігурації обладнання.
static bool pwm_config_valid(const pwm_config_t *config) {
	if (!config || config->gpio < 0 || !GPIO_IS_VALID_OUTPUT_GPIO(config->gpio)) {
		return false;
	}

	if (config->frequency_hz == 0 ||
		config->channel < LEDC_CHANNEL_0 || config->channel > LEDC_CHANNEL_7 ||
		config->timer < LEDC_TIMER_0 || config->timer > LEDC_TIMER_3 ||
		config->resolution < LEDC_TIMER_1_BIT ||
		config->resolution > LEDC_TIMER_14_BIT) {
		return false;
	}

	uint32_t max_duty = (1U << config->resolution) - 1U;
	return config->duty <= max_duty;
}

// Обчислює максимальне значення шпаруватості для заданої роздільності.
static uint32_t pwm_max_duty(ledc_timer_bit_t resolution) {
	return (1U << resolution) - 1U;
}

// Ініціалізує таймер LEDC і PWM-канал.
esp_err_t pwm_init(pwm_t *pwm, const pwm_config_t *config) {
	if (!pwm_config_valid(config)) {
		return ESP_ERR_INVALID_ARG;
	}

	if (!pwm) {
		return ESP_ERR_INVALID_ARG;
	}

	memset(pwm, 0, sizeof(*pwm));
	pwm->config = *config;
	pwm->max_duty = pwm_max_duty(config->resolution);

	ledc_timer_config_t timer_config = {0};
	timer_config.speed_mode = PWM_SPEED_MODE;
	timer_config.duty_resolution = config->resolution;
	timer_config.timer_num = config->timer;
	timer_config.freq_hz = config->frequency_hz;
	timer_config.clk_cfg = LEDC_AUTO_CLK;
	esp_err_t ret = ledc_timer_config(&timer_config);
	if (ret != ESP_OK) {
		return ret;
	}

	ledc_channel_config_t channel_config = {0};
	channel_config.gpio_num = config->gpio;
	channel_config.speed_mode = PWM_SPEED_MODE;
	channel_config.channel = config->channel;
	channel_config.intr_type = LEDC_INTR_DISABLE;
	channel_config.timer_sel = config->timer;
	channel_config.duty = config->duty;
	channel_config.hpoint = 0;
	channel_config.flags.output_invert = config->inverted;
	ret = ledc_channel_config(&channel_config);
	if (ret != ESP_OK) {
		return ret;
	}

	pwm->initialized = true;
	pwm->running = true;
	return ESP_OK;
}

// Зупиняє канал PWM і повертає відповідний GPIO у початковий стан.
esp_err_t pwm_deinit(pwm_t *pwm) {
	if (!pwm) {
		return ESP_ERR_INVALID_ARG;
	}
	if (!pwm->initialized) {
		return ESP_OK;
	}

	esp_err_t ret = ledc_stop(PWM_SPEED_MODE, pwm->config.channel, 0);
	gpio_reset_pin(pwm->config.gpio);
	memset(pwm, 0, sizeof(*pwm));
	return ret;
}

// Відновлює вихід PWM із збереженими налаштуваннями.
esp_err_t pwm_start(pwm_t *pwm) {
	if (!pwm || !pwm->initialized) {
		return ESP_ERR_INVALID_STATE;
	}

	esp_err_t ret = ledc_update_duty(PWM_SPEED_MODE, pwm->config.channel);
	if (ret == ESP_OK) {
		pwm->running = true;
	}
	return ret;
}

// Зупиняє вихід PWM.
esp_err_t pwm_stop(pwm_t *pwm) {
	if (!pwm || !pwm->initialized) {
		return ESP_ERR_INVALID_STATE;
	}

	esp_err_t ret = ledc_stop(PWM_SPEED_MODE, pwm->config.channel, 0);
	if (ret == ESP_OK) {
		pwm->running = false;
	}
	return ret;
}

// Встановлює абсолютне значення шпаруватості.
esp_err_t pwm_set_duty(pwm_t *pwm, uint32_t duty) {
	if (!pwm || !pwm->initialized) {
		return ESP_ERR_INVALID_STATE;
	}
	if (duty > pwm->max_duty) {
		return ESP_ERR_INVALID_ARG;
	}

	esp_err_t ret = ledc_set_duty(PWM_SPEED_MODE, pwm->config.channel, duty);
	if (ret == ESP_OK) {
		pwm->config.duty = duty;
		ret = ledc_update_duty(PWM_SPEED_MODE, pwm->config.channel);
	}
	return ret;
}

// Перетворює відсоткову шпаруватість у відліки LEDC.
esp_err_t pwm_set_percent(pwm_t *pwm, uint8_t percent) {
	if (percent > 100) {
		return ESP_ERR_INVALID_ARG;
	}
	if (!pwm || !pwm->initialized) {
		return ESP_ERR_INVALID_STATE;
	}

	uint32_t duty = (pwm->max_duty * percent) / 100U;
	return pwm_set_duty(pwm, duty);
}
