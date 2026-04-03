#include <ESP32Servo.h>

#define PWM_OUT  18

Servo myServo;  // Створити об'єкт сервомотора
int servoPin = PWM_OUT;  // GPIO PWM_OUT  для сигналу сервомотора
int pos = 0;  // Поточна позиція (0 градусів)

void setup() {
    Serial.begin(115200);

    // Прикріпити сервомотор до GPIO PWM_OUT
    // Діапазон PWM від 500 до 2600 мікросекунд (стандартний)
    myServo.attach(servoPin, 500, 2600);

    // Встановити початкову позицію 0°
    myServo.write(0);
    delay(500);

    Serial.println("Servo initialized at 0 degrees");
}

void loop() {
    // Розгортка від 0 до 180 градусів
    for (pos = 0; pos <= 180; pos += 5) {
        myServo.write(pos);  // Команда кута в градусах
        Serial.printf("Position: %d degrees\n", pos);
        delay(100);  // Чекати, поки мотор досягне позиції
    }

    delay(500);

    // Розгортка назад від 180 до 0 градусів
    for (pos = 180; pos >= 0; pos -= 5) {
        myServo.write(pos);
        Serial.printf("Position: %d degrees\n", pos);
        delay(100);
    }

    delay(500);
}
