#include <Arduino.h>

enum class LedState {
  OFF,
  ON,
  Blinking
};

class Config {
    private:
        uint16_t pulse_width;
        LedState states[3] = {LedState::Blinking, LedState::ON, LedState::OFF};
        constexpr static uint16_t delay_ms = 1000;
        constexpr static uint16_t pulse_widths[] = {100, 500, 1000, 2000};
        constexpr static uint8_t pulse_widths_array_size = sizeof(pulse_widths) / sizeof(pulse_widths[0]);

    public:
        Config() : pulse_width(pulse_widths[0]) {}

        void updatePulseWidths(uint8_t mode) {
            uint8_t index = mode % pulse_widths_array_size;
            this->pulse_width = pulse_widths[index];
        }

        uint16_t getPulseWidth() const {
            return this->pulse_width;
        }


        uint16_t getDelayMs() const {
            return this->delay_ms;
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
                    break;
                case LedState::ON:
                    digitalWrite(this->pin, HIGH);
                    break;
                case LedState::Blinking:
                    while (1)
                    {
                        unsigned long current_time = millis();
                        this->on();
                        while (millis() - current_time < blink_delay) {
                            // Wait for the specified high pulse width
                        }
                        this->off();
                        current_time = millis();
                        while (millis() - current_time < blink_delay) {
                            // Wait for the specified low pulse width
                        }
                    }
                    break;
            };
            this->state = new_state;
        }

    public:
        Led(uint8_t pin, uint16_t blink_delay) : state(LedState::OFF), pin(pin), blink_delay(blink_delay) {
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
};

constexpr uint8_t button_mode_up_pin = 5;
constexpr uint8_t green_led_pin = 13;
constexpr uint8_t yellow_led_pin = 4;
volatile bool button_pressed = false;
Config config;
Led greenLed(green_led_pin, config.getDelayMs());
Led yellowLed(yellow_led_pin, config.getDelayMs());

void IRAM_ATTR reaction_left() {
  button_pressed = true;
}

void setup() {
  pinMode(button_mode_up_pin, INPUT_PULLDOWN);
  Serial.begin(115200);
  attachInterrupt(digitalPinToInterrupt(button_mode_up_pin), reaction_left, RISING);
}

void loop() {
}
