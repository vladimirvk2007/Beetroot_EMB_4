#include <Arduino.h>
#include "buttonFSM.h"

#define BUTTON_PIN 			15
#define LED_PIN 			16
#define DEBOUNCE_DELAY 		50

static Button_FSM_t buttonFsm;
static int presseCount = 0;

void onButtonPress(void* arg) {
	int* pressCount = (int*)arg;
	(*pressCount)++;
	Serial.print("Button pressed: ");
	Serial.println(*pressCount);
	digitalWrite(LED_PIN, HIGH);
}

void onButtonRelease(void* arg) {
	Serial.print("Button released");
	digitalWrite(LED_PIN, LOW);
}

void setup() {
	Serial.begin(115200);
	pinMode(BUTTON_PIN, INPUT_PULLUP);
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW);

	if (Button_FSM_Init(&buttonFsm, BUTTON_PIN, DEBOUNCE_DELAY, onButtonPress, onButtonRelease, &presseCount) != 0) {
		Serial.println("Failed to initialize button FSM");
	}
}

void loop() {
	Button_FSM_Update(&buttonFsm);

	delay(1);
}
