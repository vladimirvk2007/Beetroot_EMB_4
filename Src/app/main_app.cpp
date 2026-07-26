#include <stdio.h>
#include "main.h"
#include "printf/usb_printf.h"
#include "led/led.h"

#define LED_OUT GPIO_PIN_13
#define LED_PORT GPIOC

extern "C" void main_cpp() {
    Led led13(LED_PORT, LED_OUT, false);

    while(1) {
        led13.toggle();
        printf("%s", "LED blinked\n");
        HAL_Delay(1000);
    }
}
