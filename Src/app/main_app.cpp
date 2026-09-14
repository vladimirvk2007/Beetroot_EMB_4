#include <stdio.h>
#include <stdbool.h>
#include "main.h"
#include "adc/adc.h"
#include "printf/usb_printf.h"

#define LED_INBUILT_PIN GPIO_PIN_13
#define LED_INBUILT_PORT GPIOC

extern "C" void main_cpp() {
    uint32_t adc_value;

    while(1) {
        HAL_GPIO_WritePin(LED_INBUILT_PORT, LED_INBUILT_PIN, GPIO_PIN_SET);
	    HAL_Delay(500);
	    HAL_GPIO_WritePin(LED_INBUILT_PORT, LED_INBUILT_PIN, GPIO_PIN_RESET);
	    HAL_Delay(500);

        if (ADC_Read(&adc_value, 100) == HAL_OK) {
            printf("ADC PA5: %lu\n", adc_value);
        }
    }
}
