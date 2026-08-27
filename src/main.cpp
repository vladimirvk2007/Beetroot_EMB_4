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
static ledControl_t led_1;
static Counter_t counter = {0, 0};

void onButtonPress(void* arg) {
	Counter_t* counter = (Counter_t*)arg;
	counter->presseCount++;
	Serial.print("Button pressed: ");
	Serial.println(counter->presseCount);
	ledControl_init(&led_1, LED_PIN, true, LED_MODE_BLINK, 200, 200);
}

void onButtonRelease(void* arg) {
	int retVal = 0;
	Counter_t* counter = (Counter_t*)arg;
	counter->releaseCount++;
	Serial.print("Button released: ");
	Serial.println(counter->releaseCount);
	ledControl_init(&led_1, LED_PIN, true, LED_MODE_OFF, 0, 0);
}

void setup() {
	int retVal = 0;

	Serial.begin(115200);
	pinMode(BUTTON_PIN, INPUT_PULLUP);

	retVal = ledControl_init(&led_1, LED_PIN, true, LED_MODE_OFF, 0, 0);
	if (retVal != 0) {
		Serial.println("Failed to initialize LED control");
	}

	retVal = Button_FSM_Init(&buttonFsm, BUTTON_PIN, DEBOUNCE_DELAY, onButtonPress, onButtonRelease, &counter);
	if (retVal != 0) {
		Serial.println("Failed to initialize button FSM");
	}
}

void loop() {
	int retVal = 0;

	retVal = Button_FSM_Update(&buttonFsm);
	if (retVal != 0) {
		Serial.println("Failed to update button FSM");
	}

	retVal = ledControl_update(&led_1);
	if (retVal != 0) {
		Serial.println("Failed to update LED control");
	}
}
