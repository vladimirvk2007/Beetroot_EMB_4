#include <stdio.h>
#include <stdbool.h>
#include "main.h"
#include "printf/usb_printf.h"

extern "C" ADC_HandleTypeDef hadc1;

extern "C" void main_cpp() {

    while(1) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
	    HAL_Delay(500);
	    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
	    HAL_Delay(500);

        if (HAL_ADC_Start(&hadc1) == HAL_OK) {
            if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
                printf("ADC PA5: %lu\n", HAL_ADC_GetValue(&hadc1));
            }
            HAL_ADC_Stop(&hadc1);
        }
    }
}
