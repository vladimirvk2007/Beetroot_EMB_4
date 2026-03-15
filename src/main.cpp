#include <Arduino.h>
#include <atomic>
#include "ButtonFSM.h"

#define BUTTON_PIN 			15
#define LED_PIN				7
#define LED_FSM_PIN 		16
#define DEBOUNCE_TIME_MS	50

std::atomic<uint32_t> buttonCounter{0};
volatile bool buttonPressed = false;
Button_FSM_t buttonFSM = {0};
uint32_t fsmButtonPressCount = 0;
bool fsmPressed = false;

void IRAM_ATTR handleButtonInterrupt() {
	if (digitalRead(BUTTON_PIN) == LOW) {
		buttonCounter.fetch_add(1, std::memory_order_relaxed);
		buttonPressed = true;
	}
}

void buttonFSMcallback(void *arg) {
	fsmButtonPressCount++;
	fsmPressed = true;
}

void setup() {
	int err = 0;

    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(BUTTON_PIN),
						handleButtonInterrupt, FALLING);
	pinMode(LED_PIN, OUTPUT);
	pinMode(LED_FSM_PIN, OUTPUT);
	digitalWrite(LED_PIN, HIGH);
	digitalWrite(LED_FSM_PIN, HIGH);

	err = Button_FSM_Init(&buttonFSM, BUTTON_PIN, DEBOUNCE_TIME_MS,
							buttonFSMcallback, NULL);
	if (err != 0) {
		Serial.println("Error initializing button FSM");
	}
}

void loop() {
	int buttonState = digitalRead(BUTTON_PIN);
	bool fsmButtonPressed = false;
	int err = 0;

	digitalWrite(LED_PIN, buttonState);

	err = Button_FSM_Update(&buttonFSM);
	if (err != 0) {
		Serial.println("Error updating button FSM");
	}

	err = Button_FSM_If_Pressed(&buttonFSM, &fsmButtonPressed);
	if (err != 0) {
		Serial.println("Error checking button FSM state");
	}

	digitalWrite(LED_FSM_PIN, fsmButtonPressed ? LOW : HIGH);

	if (buttonPressed) {
		buttonPressed = false;
		uint32_t count = buttonCounter.load(std::memory_order_relaxed);
		Serial.printf("Button pressed %u times\n", count);
	}

	if (fsmPressed) {
		fsmPressed = false;
		Serial.printf("FSM Button pressed %u times\n", fsmButtonPressCount);
	}
}

