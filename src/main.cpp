#include <Arduino.h>
#include "buttonFSM.h"

#define BUTTON_PIN 			15
#define LED_PIN 			16
#define DEBOUNCE_DELAY 		50

typedef struct{
	int presseCount;
	int releaseCount;
} Counter_t;

static Button_FSM_t buttonFsm;
static Counter_t counter = {0, 0};

void onButtonPress(void* arg) {
	Counter_t* counter = (Counter_t*)arg;
	counter->presseCount++;
	Serial.print("Button pressed: ");
	Serial.println(counter->presseCount);
	digitalWrite(LED_PIN, HIGH);
}

void onButtonRelease(void* arg) {
	Counter_t* counter = (Counter_t*)arg;
	counter->releaseCount++;
	Serial.print("Button released: ");
	Serial.println(counter->releaseCount);
	digitalWrite(LED_PIN, LOW);
}

void setup() {
	Serial.begin(115200);
	pinMode(BUTTON_PIN, INPUT_PULLUP);
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW);

	if (Button_FSM_Init(&buttonFsm, BUTTON_PIN, DEBOUNCE_DELAY, onButtonPress, onButtonRelease, &counter) != 0) {
		Serial.println("Failed to initialize button FSM");
	}
}

void loop() {
	Button_FSM_Update(&buttonFsm);

	delay(1);
}
