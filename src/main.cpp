#include <Arduino.h>
#include "buttonFSM.h"
#include "ledControl.h"

#define BUTTON_PIN 			15
#define LED_PIN 			16
#define BLINK_TIME_MS 		200
#define DEBOUNCE_DELAY 		50

typedef struct{
	int presseCount;
	int releaseCount;
} Counter_t;

static Button_FSM_t buttonFsm;
static hw_timer_t *ledTimer = NULL;
static ledControl_t led_1;
static Counter_t counter = {0, 0};

void IRAM_ATTR onLedTimerInterrupt() {
	ledControl_update(&led_1);
}

void onButtonPress(void*) {
	counter.presseCount++;
	Serial.print("Button pressed: ");
	Serial.println(counter.presseCount);
	ledControl_init(&led_1, LED_PIN, true, LED_MODE_BLINK, BLINK_TIME_MS, BLINK_TIME_MS);
}

void onButtonRelease(void*) {
	counter.releaseCount++;
	Serial.print("Button released: ");
	Serial.println(counter.releaseCount);
	ledControl_init(&led_1, LED_PIN, true, LED_MODE_OFF, 0, 0);
}

void setup() {
	Serial.begin(115200);

	if (ledControl_init(&led_1, LED_PIN, true, LED_MODE_OFF, 0, 0) != 0) {
		Serial.println("Failed to initialize LED control");
	}

	ledTimer = timerBegin(0, 80, true);
	if (ledTimer == NULL) {
		Serial.println("Failed to initialize LED timer");
	} else {
		timerAttachInterrupt(ledTimer, onLedTimerInterrupt, true);
		timerAlarmWrite(ledTimer, 1000, true);
		timerAlarmEnable(ledTimer);
	}

	if (Button_FSM_Init(&buttonFsm, BUTTON_PIN, DEBOUNCE_DELAY,
			onButtonPress, onButtonRelease, &counter) != 0) {
		Serial.println("Failed to initialize button FSM");
	}
	Serial.println("Button interrupt initialized");
}

void loop() {
}
