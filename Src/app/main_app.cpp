#include <stdio.h>
#include <stdbool.h>
#include "main.h"
#include "adc/adc.h"
#include "printf/usb_printf.h"

#define LED_INBUILT_PIN GPIO_PIN_13
#define LED_INBUILT_PORT GPIOC

extern "C" void main_cpp() {
    const uint32_t adc_channels[] = {ADC_CHANNEL_5, ADC_CHANNEL_6};
    uint32_t adc_values[2];

    if (ADC_Init(adc_channels, 2) != HAL_OK) {
        Error_Handler();
    }

    while(1) {
        HAL_GPIO_WritePin(LED_INBUILT_PORT, LED_INBUILT_PIN, GPIO_PIN_SET);
	    HAL_Delay(500);
	    HAL_GPIO_WritePin(LED_INBUILT_PORT, LED_INBUILT_PIN, GPIO_PIN_RESET);
	    HAL_Delay(500);

        if (ADC_ReadSequence(adc_values, 2) == HAL_OK) {
            printf("ADC PA5: %lu | ADC PA6: %lu\n",
                                    adc_values[0], adc_values[1]);
        }
    }
}
