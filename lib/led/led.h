#ifndef LED_H
#define LED_H

#include "driver/gpio.h"

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

#endif