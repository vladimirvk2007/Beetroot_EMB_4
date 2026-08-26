#include <Arduino.h>
#include "buttonFSM.h"
#include "ledControl.h"

#define BUTTON_PIN 			15
#define LED_PIN 			16
#define DEBOUNCE_DELAY 		50

typedef struct{
	int presseCount;
	int releaseCount;
} Counter_t;

static Button_FSM_t buttonFsm;
static ledControl_t ledControl;
static uint8_t ledId;
static Counter_t counter = {0, 0};

void onButtonPress(void* arg) {
	Counter_t* counter = (Counter_t*)arg;
	counter->presseCount++;
	Serial.print("Button pressed: ");
	Serial.println(counter->presseCount);
	// Start fast blinking (200ms ON, 200ms OFF) when button pressed
	ledControl_setBlink(&ledControl, ledId, 200, 200);
}

void onButtonRelease(void* arg) {
	Counter_t* counter = (Counter_t*)arg;
	counter->releaseCount++;
	Serial.print("Button released: ");
	Serial.println(counter->releaseCount);
	// Turn LED off when button released
	ledControl_setMode(&ledControl, ledId, LED_MODE_OFF);
}

void setup() {
	Serial.begin(115200);
	pinMode(BUTTON_PIN, INPUT_PULLUP);

	ledControl_init(&ledControl);
	ledControl_addLed(&ledControl, LED_PIN, true, &ledId);

	if (Button_FSM_Init(&buttonFsm, BUTTON_PIN, DEBOUNCE_DELAY, onButtonPress, onButtonRelease, &counter) != 0) {
		Serial.println("Failed to initialize button FSM");
	}
}

void loop() {
	Button_FSM_Update(&buttonFsm);
	ledControl_update(&ledControl);

	delay(1);
}
