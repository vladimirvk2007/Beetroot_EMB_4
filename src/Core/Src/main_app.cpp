#include <string.h>
#include "main.h"
#include "usbd_cdc_if.h"

extern ADC_HandleTypeDef hadc1;

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

extern "C" void main_cpp() {
    Led led13(GPIOC, GPIO_PIN_13);

    char adc_msg[64];
    uint32_t adc_val = 0;

    while(1) {
        led13.toggle();
        // Start ADC conversion
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
            adc_val = HAL_ADC_GetValue(&hadc1);
        }
        HAL_ADC_Stop(&hadc1);

        // Send ADC value over USB CDC
        int len = snprintf(adc_msg, sizeof(adc_msg), "ADC1_CH1: %lu\r\n", adc_val);
        CDC_Transmit_FS((uint8_t*)adc_msg, len);

        HAL_Delay(2000);
    }
}
