#include <stdio.h>
#include <stdbool.h>
#include "main.h"
#include "printf/usb_printf.h"

extern "C" void main_cpp() {

    while(1) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
	    HAL_Delay(500);
	    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
	    HAL_Delay(500);

        printf("%s", "LED blinked\n");
    }
}
