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
    float voltage = 0.0f;
    const float VREF = 3.3f;
    const uint32_t ADC_MAX = 4095;

    while(1) {
        led13.toggle();

        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
            adc_val = HAL_ADC_GetValue(&hadc1);
            voltage = ((float)adc_val / ADC_MAX) * VREF;
        }
        HAL_ADC_Stop(&hadc1);

        int len = snprintf(adc_msg, sizeof(adc_msg), "CH1: %lu, Voltage: %.3f V\r\n",
                            adc_val, voltage);
        CDC_Transmit_FS((uint8_t*)adc_msg, len);

        HAL_Delay(1000);
    }
}
