#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED_OUT		GPIO_NUM_16
#define BUTTON_IN	GPIO_NUM_15

class Led {
public:
    Led(gpio_num_t pin) : pin_(pin) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << pin_),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_conf);
    }
    void on() const {
        gpio_set_level(pin_, 0);
    }
    void off() const {
        gpio_set_level(pin_, 1);
    }
private:
    const gpio_num_t pin_;
};

class Button {
public:
    Button(gpio_num_t pin) : pin_(pin) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << pin_),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_conf);
    }
    bool isPressed() const {
        return gpio_get_level(pin_) == 0; // active low
    }
private:
    const gpio_num_t pin_;
};

extern "C"
{

void app_main() {
    Led led(LED_OUT);
    Button button(BUTTON_IN);

    while (1) {
        if (button.isPressed()) {
            led.on();
            printf("Button Pressed\n");
        } else {
            led.off();
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

} // extern "C"

