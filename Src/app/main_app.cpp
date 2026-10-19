#include <stdio.h>
#include <stdbool.h>
#include "main.h"
#include "adc/adc.h"
#include "printf/usb_printf.h"

#define LED_INBUILT_PIN GPIO_PIN_13
#define LED_INBUILT_PORT GPIOC

static uint32_t adc_values[2];
static volatile bool adc_data_ready;
static volatile bool adc_error;

extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        adc_data_ready = true;
    }
}

extern "C" void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        adc_error = true;
    }
}

extern "C" void main_cpp() {
    const uint32_t adc_channels[] = {ADC_CHANNEL_5, ADC_CHANNEL_6};

    if (ADC_Init(adc_channels, 2) != HAL_OK) {
        Error_Handler();
    }

    if (ADC_StartSequenceDMA(adc_values, 2) != HAL_OK) {
        Error_Handler();
    }

    while(1) {
        if (adc_error) {
            Error_Handler();
        }

        if (adc_data_ready) {
            adc_data_ready = false;
            printf("ADC PA5: %lu | ADC PA6: %lu\n",
                   static_cast<unsigned long>(adc_values[0]),
                   static_cast<unsigned long>(adc_values[1]));

            if (ADC_StartSequenceDMA(adc_values, 2) != HAL_OK) {
                Error_Handler();
            }
        }

        HAL_GPIO_WritePin(LED_INBUILT_PORT, LED_INBUILT_PIN, GPIO_PIN_SET);
	    HAL_Delay(500);
	    HAL_GPIO_WritePin(LED_INBUILT_PORT, LED_INBUILT_PIN, GPIO_PIN_RESET);
	    HAL_Delay(500);
    }
}
