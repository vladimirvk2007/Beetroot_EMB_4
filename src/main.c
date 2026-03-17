
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define BLINK_GPIO 16

void app_main() {
	gpio_config_t io_conf = {
		.pin_bit_mask = (1ULL << BLINK_GPIO),
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = 0,
		.pull_down_en = 0,
		.intr_type = GPIO_INTR_DISABLE
	};
	gpio_config(&io_conf);

	while (1) {
		gpio_set_level(BLINK_GPIO, 1);
		printf("LED ON\n");
		vTaskDelay(500 / portTICK_PERIOD_MS);
		gpio_set_level(BLINK_GPIO, 0);
		printf("LED OFF\n");
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}
