#include <string.h>
#include "main.h"
#include "usbd_cdc_if.h"

#define MESSAGE_SIZE        48
#define LED_GPIO_PIN        GPIO_PIN_13
#define LED_GPIO_PORT       GPIOC
#define UART_TIMEOUT_MS     100  // Таймаут для UART в мілісекундах
#define UART_BUFFER_SIZE    1    // Розмір буфера для прийому даних з UART
#define LED_BLINK_DELAY_MS  10   // Затримка для блимання світлодіода в мілісекундах

extern UART_HandleTypeDef huart2;  // USART2: PA3 (RX), PA2 (TX)

class Led {
public:
    Led(GPIO_TypeDef* port, uint16_t pin) : port_(port), pin_(pin) {}
    void toggle() { HAL_GPIO_TogglePin(port_, pin_); }
    void on()    { HAL_GPIO_WritePin(port_, pin_, GPIO_PIN_SET); }
    void off()   { HAL_GPIO_WritePin(port_, pin_, GPIO_PIN_RESET); }
    void blink(uint32_t delay_ms) {
        on();
        HAL_Delay(delay_ms);
        off();
        HAL_Delay(delay_ms);
    }
private:
    GPIO_TypeDef* port_;
    const uint16_t pin_;
};

extern "C" void main_cpp() {
    Led led13(LED_GPIO_PORT, LED_GPIO_PIN);
    uint8_t msg[MESSAGE_SIZE] = {0};
    uint8_t rxData[UART_BUFFER_SIZE];

    while(1) {
        // Неблокуючий прийом із таймаутом 0 (проверка без очікування)
        if (HAL_UART_Receive(&huart2, rxData, UART_BUFFER_SIZE, 0) == HAL_OK) {
            // Отримали 1 байт
            sprintf((char*)msg, "Received from UART2: 0x%x\r\n", rxData[0]);
            CDC_Transmit_FS(msg, strlen((char*)msg));
            // Відправляємо отриманий байт назад на UART
            HAL_UART_Transmit(&huart2, (uint8_t*)rxData, UART_BUFFER_SIZE, UART_TIMEOUT_MS);
            // Блимання світлодіода для візуального підтвердження отримання даних
            led13.blink(LED_BLINK_DELAY_MS);
        }
    }
}
