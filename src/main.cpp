#include <Arduino.h>

enum class LedState {
    OFF,
    ON,
    Blinking
};

constexpr uint8_t button_mode_up_pin = 5; // Константи краще великеми літерами
constexpr uint8_t green_led_pin = 13;
volatile unsigned long lastInterruptTime = 0;
volatile bool button_pressed = false;

class Config {
    private:
        constexpr static uint16_t blinking_time = 5000;
        constexpr static uint16_t blink_delay = 100;
        constexpr static uint16_t debounce_time = 50;

    public:
        static uint16_t getBlinkDelay() {
            return Config::blink_delay;
        }

        static uint16_t getBlinkingTime() {
            return Config::blinking_time;
        }

        static uint16_t getDebounceTime() {
            return Config::debounce_time;
        }
};

class Led {
    private:
        LedState state;
        uint8_t pin;
        uint16_t blink_delay;

        void setState(LedState new_state) {

            switch(new_state) {
                case LedState::OFF:
                    digitalWrite(this->pin, LOW);
                    this->state = new_state; // Навіщо повторювати це в кожному кейсі?
                    break;
                case LedState::ON:
                    digitalWrite(this->pin, HIGH);
                    this->state = new_state; // Навіщо повторювати це в кожному кейсі? Можна винести за межі switch
                    break;
                case LedState::Blinking: { // Краще зробити окрему функцію для блимання, і викликати її з loop(), щоб не блокувати виконання програми.
                    unsigned long time_before_blinking = millis();
                    this->state = new_state; // Навіщо повторювати це в кожному кейсі?
                    while (!button_pressed && millis() - time_before_blinking < Config::getBlinkingTime()) // Не треба робити великий вираз в умові, краще винести це в окрему змінну.
                    { // button_pressed - ніяк не може бути в цьому класі. LED не може знати про кнопку.
                        unsigned long current_time = millis();
                        digitalWrite(this->pin, HIGH);
                        while (millis() - current_time < Config::getBlinkDelay()) { // Втрачається сенс використання millis().
                            // Wait for the specified high pulse width
                        }
                        digitalWrite(this->pin, LOW);
                        current_time = millis();
                        while (millis() - current_time < Config::getBlinkDelay()) { // Втрачається сенс використання millis().
                            // Wait for the specified low pulse width
                        }
                    }
                    break;
                }
            };
        }

    public:
        Led() : state(LedState::OFF), pin(0), blink_delay(0) {} // Чому нема налаштування 0-го GPIO?

        Led(uint8_t pin) : state(LedState::OFF), pin(pin), blink_delay(0) {
            pinMode(pin, OUTPUT);
        }

        void on() {
            this->setState(LedState::ON); // this тут не обов'язковий
        }

        void off() {
            this->setState(LedState::OFF); // this тут не обов'язковий
        }

        void blink() {
            this->setState(LedState::Blinking); // this тут не обов'язковий
        }

        LedState getState() {
            return this->state; // this тут не обов'язковий
        }
};

Led greenLed;

void IRAM_ATTR button_pressed_isr() {
    unsigned long currentTime = millis();

    // Ігноруємо переривання, що надходять занадто часто (брязкіт)
    if (currentTime - lastInterruptTime > Config::getDebounceTime()) {
        lastInterruptTime = currentTime;
        button_pressed = true;
    }
}

void setup() {
  pinMode(button_mode_up_pin, INPUT_PULLDOWN);
  Serial.begin(115200);
  greenLed = Led(green_led_pin);
  attachInterrupt(digitalPinToInterrupt(button_mode_up_pin), button_pressed_isr, RISING);
}

void loop() {
    if (button_pressed) {
        Serial.println("Button pressed!");
        button_pressed = false;

        switch (greenLed.getState())
        {
            case LedState::OFF:
                greenLed.blink(); // Зразу і не зрозумів, що йде зміна стану
                break;
            case LedState::ON:
                greenLed.off();
                break;
            case LedState::Blinking:
                greenLed.on();
                break;
        }
    }
}

/*Висновок:
Програма працює. Створений окремий клас Led, для організаціі логіки.
Але маю зауваження:
1. Константи краще великеми літерами, щоб було зрозуміло, що це константи.
2. Кейс Blinking блокує виконання програми. Краще зробити окрему функцію для блимання, і викликати її з loop().
3. В класі Led використовується змінна button_pressed - це порушує принцип інкапсуляції.
4. В конструкторі Led, відсутннє налаштування 0-го GPIO.
5. Використання this в методах класу не є обов'язковим, якщо немає конфлікту імен.
6. І головне, неявна логіка переключення станів в switch (greenLed.getState()). Я зразу і не зрозумів.
Будь ласка, намагайся зробити програму більш читабельною і логічною.
*/
