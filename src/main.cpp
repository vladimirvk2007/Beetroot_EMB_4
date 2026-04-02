#include <ESP32Servo.h>

#define PWM_OUT  18

Servo myServo;  // Створити об'єкт сервомотора
int servoPin = PWM_OUT;  // GPIO PWM_OUT  для сигналу сервомотора
int pos = 0;  // Поточна позиція (0-180 градусів)

void setup() {
    Serial.begin(115200);

    // Прикріпити сервомотор до GPIO18
    // Діапазон PWM від 1000 до 2000 мікросекунд (стандартний)
    myServo.attach(servoPin, 1000, 2000);

    // Встановити початкову позицію 90°
    myServo.write(90);
    delay(500);

    Serial.println("Servo initialized at 90 degrees");
}

void loop() {
    // Розгортка від 0 до 180 градусів
    for (pos = 0; pos <= 180; pos += 5) {
        myServo.write(pos);  // Команда кута в градусах
        Serial.printf("Position: %d degrees\n", pos);
        delay(100);  // Чекати, поки мотор досягне позиції
    }

    // Розгортка назад від 180 до 0 градусів
    for (pos = 180; pos >= 0; pos -= 5) {
        myServo.write(pos);
        Serial.printf("Position: %d degrees\n", pos);
        delay(100);
    }

    delay(1000);  // Пауза перед наступним циклом
}