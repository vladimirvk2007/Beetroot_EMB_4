#include "sine.h"
#include "esp_log.h"

static const char *TAG = "sine";

static const uint8_t sine_percent[SINE_TABLE_SIZE] = {
	50, 60, 69, 77, 85, 91, 96, 99,
	100, 99, 96, 91, 85, 77, 69, 60,
	50, 40, 31, 23, 15, 9, 4, 1,
	0, 1, 4, 9, 15, 23, 31, 40,
};

static void sine_timer_callback(void *arg) {
	sine_t *sine = static_cast<sine_t *>(arg);
	if (!sine || !sine->pwm || !sine->running) {
		return;
	}

	pwm_set_percent(sine->pwm, sine_percent[sine->table_index]);
	sine->table_index = (sine->table_index + 1) % SINE_TABLE_SIZE;
}

static esp_err_t sine_set_safe_pwm_config(sine_t *sine, uint32_t requested_hz) {
	if (!sine || !sine->pwm || requested_hz == 0) {
		return ESP_ERR_INVALID_ARG;
	}

	uint32_t requested_pwm_hz = requested_hz * SINE_TABLE_SIZE;
	if (requested_pwm_hz <= pwm_max_frequency(sine->pwm->config.resolution)) {
		return pwm_set_frequency(sine->pwm, requested_pwm_hz);
	}

	for (int res_index = sine->pwm->config.resolution;
		res_index >= LEDC_TIMER_1_BIT; --res_index) {
		ledc_timer_bit_t resolution = static_cast<ledc_timer_bit_t>(res_index);
		uint32_t max_frequency = pwm_max_frequency(resolution);
		if (requested_pwm_hz <= max_frequency) {
			if (resolution != sine->pwm->config.resolution) {
				ledc_timer_bit_t previous_resolution = sine->pwm->config.resolution;
				esp_err_t ret = pwm_set_resolution(sine->pwm, resolution);
				if (ret != ESP_OK) {
					continue;
				}
				ESP_LOGI(TAG, "PWM resolution changed: %d bit -> %d bit",
					 previous_resolution, resolution);
			}

			uint32_t previous_pwm_hz = sine->pwm->config.frequency_hz;
			esp_err_t ret = pwm_set_frequency(sine->pwm, requested_pwm_hz);
			if (ret == ESP_OK && previous_pwm_hz != requested_pwm_hz) {
				ESP_LOGI(TAG, "PWM frequency changed: %lu Hz -> %lu Hz",
					 previous_pwm_hz, requested_pwm_hz);
			}
			return ret;
		}
	}

	return ESP_ERR_INVALID_ARG;
}

static esp_err_t sine_update_period(sine_t *sine, uint32_t frequency_hz) {
	if (!sine || frequency_hz == 0 ||
		frequency_hz > 1000000 / SINE_TABLE_SIZE) {
		return ESP_ERR_INVALID_ARG;
	}

	if (sine_set_safe_pwm_config(sine, frequency_hz) != ESP_OK) {
		return ESP_ERR_INVALID_ARG;
	}

	uint32_t sample_period_us =
		1000000 / (frequency_hz * SINE_TABLE_SIZE);
	if (sample_period_us == 0) {
		return ESP_ERR_INVALID_ARG;
	}

	sine->frequency_hz = frequency_hz;
	sine->pwm_frequency_hz = sine->pwm->config.frequency_hz;
	sine->sample_period_us = sample_period_us;
	return ESP_OK;
}

esp_err_t sine_init(sine_t *sine, pwm_t *pwm, uint32_t frequency_hz) {
	if (!sine || !pwm || !pwm->initialized) {
		return ESP_ERR_INVALID_ARG;
	}

	*sine = {};
	sine->pwm = pwm;
	if (sine_update_period(sine, frequency_hz) != ESP_OK) {
		return ESP_ERR_INVALID_ARG;
	}

	esp_timer_create_args_t timer_args = {};
	timer_args.callback = sine_timer_callback;
	timer_args.arg = sine;
	timer_args.dispatch_method = ESP_TIMER_TASK;
	timer_args.name = "sine";

	esp_err_t ret = esp_timer_create(&timer_args, &sine->timer);
	if (ret != ESP_OK) {
		return ret;
	}

	sine->initialized = true;
	return ESP_OK;
}

esp_err_t sine_deinit(sine_t *sine) {
	if (!sine) {
		return ESP_ERR_INVALID_ARG;
	}
	if (!sine->initialized) {
		return ESP_OK;
	}

	esp_err_t ret = sine_stop(sine);
	if (ret != ESP_OK) {
		return ret;
	}

	ret = esp_timer_delete(sine->timer);
	if (ret == ESP_OK) {
		*sine = {};
	}
	return ret;
}

esp_err_t sine_start(sine_t *sine) {
	if (!sine || !sine->initialized) {
		return ESP_ERR_INVALID_STATE;
	}
	if (sine->running) {
		return ESP_OK;
	}

	sine->table_index = 0;
	sine->running = true;
	esp_err_t ret = esp_timer_start_periodic(sine->timer, sine->sample_period_us);
	if (ret != ESP_OK) {
		sine->running = false;
	}
	return ret;
}

esp_err_t sine_stop(sine_t *sine) {
	if (!sine || !sine->initialized) {
		return ESP_ERR_INVALID_STATE;
	}
	if (!sine->running) {
		return ESP_OK;
	}

	sine->running = false;
	esp_err_t ret = esp_timer_stop(sine->timer);
	if (ret == ESP_OK) {
		ret = pwm_set_percent(sine->pwm, 0);
	}
	return ret;
}

esp_err_t sine_set_frequency(sine_t *sine, uint32_t frequency_hz) {
	if (!sine || !sine->initialized) {
		return ESP_ERR_INVALID_STATE;
	}

	bool was_running = sine->running;
	if (was_running) {
		esp_err_t ret = sine_stop(sine);
		if (ret != ESP_OK) {
			return ret;
		}
	}

	esp_err_t ret = sine_update_period(sine, frequency_hz);
	if (ret == ESP_OK && was_running) {
		ret = sine_start(sine);
	}
	return ret;
}
