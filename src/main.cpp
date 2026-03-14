#include <Arduino.h>

enum class LedState {
    OFF,
    ON,
    Blinking
};

class Config {
    private:
        constexpr static uint16_t delay_ms = 1000;

    public:
        Config() {}

        static uint16_t getDelayMs() {
            return delay_ms;
        }
};

class Led {
    private:
        LedState state;
        uint8_t pin;
        uint16_t blink_delay;

        volatile void setState(LedState new_state) {

            switch(new_state) {
                case LedState::OFF:
                    digitalWrite(this->pin, LOW);
                    break;
                case LedState::ON:
                    digitalWrite(this->pin, HIGH);
                    break;
                case LedState::Blinking:
                    while (button_pressed == false)
                    {
                        unsigned long current_time = millis();
                        this->on();
                        while (millis() - current_time < blink_delay) {
                            // Wait for the specified high pulse width
                        }
                        this->off();
                        current_time = millis();
                        while (millis() - current_time < Config::getDelayMs()) {
                            // Wait for the specified low pulse width
                        }
                    }
                    break;
            };
            this->state = new_state;
        }

    public:
        Led(uint8_t pin) : state(LedState::OFF), pin(pin) {
            pinMode(pin, OUTPUT);
        }

        void on() {
            this->setState(LedState::ON);
        }

        void off() {
            this->setState(LedState::OFF);
        }

        void blink() {
            this->setState(LedState::Blinking);
        }

        LedState getState() {
            return this->state;
        }
};

constexpr uint8_t button_mode_up_pin = 5;
constexpr uint8_t green_led_pin = 13;
constexpr uint8_t yellow_led_pin = 4;
unsigned long lastInterruptTime = 0;
volatile bool button_pressed = false;
Led greenLed;
Led yellowLed;

void IRAM_ATTR button_pressed_isr() {
    unsigned long currentTime = millis();

    // Ігноруємо переривання, що надходять занадто часто (брязкіт)
    if (currentTime - lastInterruptTime > 50) {
        lastInterruptTime = currentTime;
        button_pressed = true;
    }
}

void setup() {
  pinMode(button_mode_up_pin, INPUT_PULLDOWN);
  Serial.begin(115200);
  greenLed = Led(green_led_pin);
  yellowLed = Led(yellow_led_pin);
  attachInterrupt(digitalPinToInterrupt(button_mode_up_pin), button_pressed_isr, RISING);
}

void loop() {
    if (button_pressed) {
        Serial.println("Button pressed!");
        button_pressed = false;

        switch (greenLed.getState())
        {
            case LedState::OFF:
                greenLed.blink();
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
