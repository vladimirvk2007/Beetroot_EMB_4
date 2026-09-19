#include "sound.h"

const uint8_t sound_sine_percent[SOUND_SINE_TABLE_SIZE] = {
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
		static_cast<uint8_t>((sound->table_index + 1) % SOUND_SINE_TABLE_SIZE);
}

static uint32_t sound_pwm_max_frequency(const pwm_t *pwm) {
	if (!pwm) {
		return 0U;
	}
	return 80000000U / (1U << pwm->config.resolution);
}

static esp_err_t sound_update_period(sound_t *sound, uint32_t frequency_hz) {
	if (!sound || frequency_hz == 0 || frequency_hz > 1500) {
		return ESP_ERR_INVALID_ARG;
	}

	uint32_t pwm_steps = 1U << sound->pwm->config.resolution;
	uint32_t pwm_frequency_hz = frequency_hz * pwm_steps;
	if (pwm_frequency_hz > sound_pwm_max_frequency(sound->pwm)) {
		return ESP_ERR_INVALID_ARG;
	}

	uint32_t sample_period_us =
		1000000U / (frequency_hz * SOUND_SINE_TABLE_SIZE);
	if (sample_period_us == 0) {
		return ESP_ERR_INVALID_ARG;
	}

	sound->frequency_hz = frequency_hz;
	sound->pwm_frequency_hz = pwm_frequency_hz;
	sound->sample_period_us = sample_period_us;
	return pwm_set_frequency(sound->pwm, pwm_frequency_hz);
}

esp_err_t sound_init(sound_t *sound, pwm_t *pwm, uint32_t frequency_hz) {
	if (!sound || !pwm || !pwm->initialized) {
		return ESP_ERR_INVALID_ARG;
	}
	if (pwm->config.resolution < LEDC_TIMER_1_BIT ||
		pwm->config.resolution > LEDC_TIMER_14_BIT) {
		return ESP_ERR_INVALID_ARG;
	}
	if (sound_pwm_max_frequency(pwm) == 0U) {
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
