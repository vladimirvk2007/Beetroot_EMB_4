#include <Arduino.h>

enum class LedState {
  OFF,
  ON
};

class Config {
    private:
        uint16_t pulse_width_high;
        uint16_t pulse_width_low;
        uint16_t delay_ms = 1000;
        constexpr static uint16_t pulse_widths[] = {100, 500, 1000, 2000};
        constexpr static uint8_t pulse_widths_array_size = sizeof(pulse_widths) / sizeof(pulse_widths[0]);

    public:
        Config() : pulse_width_high(pulse_widths[0]), pulse_width_low(pulse_widths[0]) {}

        void updatePulseWidths(uint8_t mode) {
            uint8_t index = mode % pulse_widths_array_size;
            this->pulse_width_high = pulse_widths[index];
            this->pulse_width_low = pulse_widths[index];
        }

        uint16_t getPulseWidthHigh() const {
            return this->pulse_width_high;
        }

        uint16_t getPulseWidthLow() const {
            return this->pulse_width_low;
        }

        uint16_t getDelayMs() const {
            return this->delay_ms;
        }
};

class Led {
    private:
        LedState state;
        uint8_t pin;

        void setState(LedState new_state) {
            if (new_state == LedState::ON) {
                digitalWrite(this->pin, HIGH);
            } else {
                digitalWrite(this->pin, LOW);
            }
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

        void blink(uint16_t pulse_width_high, uint16_t pulse_width_low) {
            unsigned long current_time = millis();
            this->on();
            while (millis() - current_time < pulse_width_high) {
                // Wait for the specified high pulse width
            }
            this->off();
            current_time = millis();
            while (millis() - current_time < pulse_width_low) {
                // Wait for the specified low pulse width
            }
        }
};

constexpr uint16_t delay_ms = 1000;
constexpr uint8_t button_mode_up = 5;
constexpr uint8_t green_led = 13;
constexpr uint8_t yellow_led = 4;
volatile bool button_pressed = false;
Led greenLed(green_led);
Led yellowLed(yellow_led);
Config config;

void IRAM_ATTR reaction_left() {
  button_pressed = true;
}

void setup() {
  pinMode(button_mode_up, INPUT_PULLDOWN);
  Serial.begin(115200);
  attachInterrupt(digitalPinToInterrupt(button_mode_up), reaction_left, RISING);
}

void loop() {
  if (button_pressed) {
    button_pressed = false;
    yellowLed.on();
    unsigned long current_time = millis();
    yellowLed.on();
    greenLed.on();
    while (millis() - current_time < config.getDelayMs()) {
        // Wait for the specified high pulse width
    }
    yellowLed.off();
    greenLed.off();
    current_time = millis();
    while (millis() - current_time < config.getDelayMs()) {
        // Wait for the specified low pulse width
    }
  }

  greenLed.blink(config.getPulseWidthHigh(), config.getPulseWidthLow());
  yellowLed.blink(config.getPulseWidthHigh(), config.getPulseWidthLow());
}
