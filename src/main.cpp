#include <Arduino.h>
#include <HardwareSerial.h>

#define UART1_TX_PIN 17
#define UART1_RX_PIN 18

// Використовуємо UART1 на пінах GPIO17 (TX) та GPIO18 (RX)
HardwareSerial uart1(1);

void setup() {
    Serial.begin(115200);     // Монітор через USB (UART0)
    uart1.begin(9600, SERIAL_8N1, UART1_RX_PIN, UART1_TX_PIN);  // UART1: 9600 бод, 8N1, RX=18, TX=17
    Serial.println("ESP32-S3 UART is ready to communicate!");
}

void loop() {
    if (uart1.available()) {
        byte ch = uart1.read();
        Serial.print("Received UART1: ");
        Serial.println(ch, HEX);  // Виводимо у HEX для діагностики
    }

    // Можна також надсилати дані
    if (Serial.available()) {
        byte cmd = Serial.read();
        Serial.print("Received from Serial Monitor: ");
        Serial.println(cmd, HEX);  // Виводимо команду з Serial Monitor у HEX
        uart1.write(cmd);  // Передаємо команду з Serial Monitor на UART1
    }
}

