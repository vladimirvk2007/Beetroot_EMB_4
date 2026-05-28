#ifndef LED_H
#define LED_H

#define LED_ON_LEVEL GPIO_PIN_RESET
#define LED_OFF_LEVEL GPIO_PIN_SET

class Led {
public:
    Led(GPIO_TypeDef* port, uint16_t pin) : port_(port), pin_(pin) {
        GPIO_InitTypeDef init = {0};
        init.Pin = pin_;
        init.Mode = GPIO_MODE_OUTPUT_PP;
        init.Pull = GPIO_NOPULL;
        init.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(port_, &init);
        off();
    }

    void on() const {
        HAL_GPIO_WritePin(port_, pin_, LED_ON_LEVEL);
    }

    void off() const {
        HAL_GPIO_WritePin(port_, pin_, LED_OFF_LEVEL);
    }

    void toggle() const {
        HAL_GPIO_TogglePin(port_, pin_);
    }

private:
    GPIO_TypeDef* port_;
    uint16_t pin_;
};

#endif // LED_H
