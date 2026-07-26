#ifndef LED_H
#define LED_H

#include "stm32f4xx_hal.h"

class Led {
public:
    Led(GPIO_TypeDef* port, uint16_t pin, bool active_high = true)
        : port_(port), pin_(pin), active_high_(active_high) {}
    void toggle()
    {
        HAL_GPIO_TogglePin(port_, pin_);
    }
    void on()
    {
        HAL_GPIO_WritePin(port_, pin_, active_high_ ?
                                    GPIO_PIN_SET : GPIO_PIN_RESET);
    }
    void off()
    {
        HAL_GPIO_WritePin(port_, pin_, active_high_ ?
                                    GPIO_PIN_RESET : GPIO_PIN_SET);
    }
private:
    GPIO_TypeDef* port_;
    const uint16_t pin_;
    const bool active_high_;
};

#endif /* LED_H */
