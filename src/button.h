#ifndef BUTTON_H
#define BUTTON_H

#include "driver/gpio.h"

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


#endif // BUTTON_H

