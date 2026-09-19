#include "sound.h"
#include "esp_log.h"

static const char *TAG = "sound";

static const uint8_t sound_sine_percent[SOUND_SINE_TABLE_SIZE] = {
	50, 60, 69, 77, 85, 91, 96, 99,
	100, 99, 96, 91, 85, 77, 69, 60,
	50, 40, 31, 23, 15, 9, 4, 1,
	0, 1, 4, 9, 15, 23, 31, 40,
};

static void sound_timer_callback(void *arg) {
	sound_t *sound = static_cast<sound_t *>(arg);
	if (!sound || !sound->pwm || !sound->running) {
		return;
	}

	pwm_set_percent(sound->pwm, sound_sine_percent[sound->table_index]);
	sound->table_index =
		(sound->table_index + 1) % SOUND_SINE_TABLE_SIZE;
}

static esp_err_t sound_set_safe_pwm_config(sound_t *sound, uint32_t requested_hz) {
	if (!sound || !sound->pwm || requested_hz == 0) {
		return ESP_ERR_INVALID_ARG;
	}

	uint32_t requested_pwm_hz = requested_hz * SOUND_SINE_TABLE_SIZE;
	if (requested_pwm_hz <= pwm_max_frequency(sound->pwm->config.resolution)) {
		return pwm_set_frequency(sound->pwm, requested_pwm_hz);
	}

	for (int res_index = sound->pwm->config.resolution;
								res_index >= LEDC_TIMER_1_BIT; --res_index) {
		ledc_timer_bit_t resolution = static_cast<ledc_timer_bit_t>(res_index);
		uint32_t max_frequency = pwm_max_frequency(resolution);
		if (requested_pwm_hz <= max_frequency) {
			if (resolution != sound->pwm->config.resolution) {
				ledc_timer_bit_t previous_resolution = sound->pwm->config.resolution;
				esp_err_t ret = pwm_set_resolution(sound->pwm, resolution);
				if (ret != ESP_OK) {
					continue;
				}
				ESP_LOGI(TAG, "PWM resolution changed: %d bit -> %d bit",
					 previous_resolution, resolution);
			}

			uint32_t previous_pwm_hz = sound->pwm->config.frequency_hz;
		 esp_err_t ret = pwm_set_frequency(sound->pwm, requested_pwm_hz);
			if (ret == ESP_OK && previous_pwm_hz != requested_pwm_hz) {
				ESP_LOGI(TAG, "PWM frequency changed: %lu Hz -> %lu Hz",
					 previous_pwm_hz, requested_pwm_hz);
			}
			return ret;
		}
	}

	return ESP_ERR_INVALID_ARG;
}

static esp_err_t sound_update_period(sound_t *sound, uint32_t frequency_hz) {
	if (!sound || frequency_hz == 0 ||
		frequency_hz > 1000000 / SOUND_SINE_TABLE_SIZE) {
		return ESP_ERR_INVALID_ARG;
	}

	if (sound_set_safe_pwm_config(sound, frequency_hz) != ESP_OK) {
		return ESP_ERR_INVALID_ARG;
	}

	uint32_t sample_period_us =
		1000000 / (frequency_hz * SOUND_SINE_TABLE_SIZE);
	if (sample_period_us == 0) {
		return ESP_ERR_INVALID_ARG;
	}

	sound->frequency_hz = frequency_hz;
	sound->pwm_frequency_hz = sound->pwm->config.frequency_hz;
	sound->sample_period_us = sample_period_us;
	return ESP_OK;
}

esp_err_t sound_init(sound_t *sound, pwm_t *pwm, uint32_t frequency_hz) {
	if (!sound || !pwm || !pwm->initialized) {
		return ESP_ERR_INVALID_ARG;
	}

	*sound = {};
	sound->pwm = pwm;
	if (sound_update_period(sound, frequency_hz) != ESP_OK) {
		return ESP_ERR_INVALID_ARG;
	}

	esp_timer_create_args_t timer_args = {};
	timer_args.callback = sound_timer_callback;
	timer_args.arg = sound;
	timer_args.dispatch_method = ESP_TIMER_TASK;
	timer_args.name = "sound_sine";

	esp_err_t ret = esp_timer_create(&timer_args, &sound->timer);
	if (ret != ESP_OK) {
		return ret;
	}

	sound->initialized = true;
	return ESP_OK;
}

esp_err_t sound_deinit(sound_t *sound) {
	if (!sound) {
		return ESP_ERR_INVALID_ARG;
	}
	if (!sound->initialized) {
		return ESP_OK;
	}

	esp_err_t ret = sound_stop(sound);
	if (ret != ESP_OK) {
		return ret;
	}

	ret = esp_timer_delete(sound->timer);
	if (ret == ESP_OK) {
		*sound = {};
	}
	return ret;
}

esp_err_t sound_start(sound_t *sound) {
	if (!sound || !sound->initialized) {
		return ESP_ERR_INVALID_STATE;
	}
	if (sound->running) {
		return ESP_OK;
	}

	sound->table_index = 0;
	sound->running = true;
	esp_err_t ret = esp_timer_start_periodic(sound->timer, sound->sample_period_us);
	if (ret != ESP_OK) {
		sound->running = false;
	}
	return ret;
}

esp_err_t sound_stop(sound_t *sound) {
	if (!sound || !sound->initialized) {
		return ESP_ERR_INVALID_STATE;
	}
	if (!sound->running) {
		return ESP_OK;
	}

	sound->running = false;
	esp_err_t ret = esp_timer_stop(sound->timer);
	if (ret == ESP_OK) {
		ret = pwm_set_percent(sound->pwm, 0);
	}
	return ret;
}

esp_err_t sound_set_frequency(sound_t *sound, uint32_t frequency_hz) {
	if (!sound || !sound->initialized) {
		return ESP_ERR_INVALID_STATE;
	}

	bool was_running = sound->running;
	if (was_running) {
		esp_err_t ret = sound_stop(sound);
		if (ret != ESP_OK) {
			return ret;
		}
	}

	esp_err_t ret = sound_update_period(sound, frequency_hz);
	if (ret == ESP_OK && was_running) {
		ret = sound_start(sound);
	}
	return ret;
}
