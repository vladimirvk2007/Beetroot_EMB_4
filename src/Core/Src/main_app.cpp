#include <string.h>
#include "main.h"
#include "usbd_cdc_if.h"
#include "led.h"


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