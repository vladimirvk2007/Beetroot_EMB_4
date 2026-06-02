#include <Arduino.h>
#include <atomic>
#include "ButtonFSM.h"

#define BUTTON_PIN 			15
#define LED_PIN				7
#define LED_FSM_PIN 		16
#define DEBOUNCE_TIME_MS	200

Button_FSM_t buttonFSM = {0};
uint32_t fsmButtonPressCount = 0;
bool fsmPressed = false;

void buttonFSMcallback(void *arg) {
	fsmButtonPressCount++;
	fsmPressed = true;
}

void setup() {
	int err = 0;

    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
	pinMode(LED_FSM_PIN, OUTPUT);
	digitalWrite(LED_FSM_PIN, LOW);

	err = Button_FSM_Init(&buttonFSM, BUTTON_PIN, DEBOUNCE_TIME_MS,
							buttonFSMcallback, NULL);
	if (err != 0) {
		Serial.println("Error initializing button FSM");
	}
}

void loop() {
	int err = 0;
	bool fsmButtonPressed = false;

	err = Button_FSM_Update(&buttonFSM);
	if (err != 0) {
		Serial.println("Error updating button FSM");
	}

	err = Button_FSM_If_Pressed(&buttonFSM, &fsmButtonPressed);
	if (err != 0) {
		Serial.println("Error checking button FSM state");
	}

	digitalWrite(LED_FSM_PIN, fsmButtonPressed ? HIGH : LOW);
}
