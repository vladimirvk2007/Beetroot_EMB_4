#include <string.h>
#include "main.h"
#include "usbd_cdc_if.h"

class Led {
public:
    Led(GPIO_TypeDef* port, uint16_t pin) : port_(port), pin_(pin) {}
    void toggle() { HAL_GPIO_TogglePin(port_, pin_); }
    void on()    { HAL_GPIO_WritePin(port_, pin_, GPIO_PIN_SET); }
    void off()   { HAL_GPIO_WritePin(port_, pin_, GPIO_PIN_RESET); }
private:
    GPIO_TypeDef* port_;
    const uint16_t pin_;
};

extern "C" {

void main_cpp() {
    const char *msg = "LED blinked!\r\n";
    Led led13(GPIOC, GPIO_PIN_13);

    while(1) {
        led13.toggle();
        CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
        HAL_Delay(2000);
    }
}

}