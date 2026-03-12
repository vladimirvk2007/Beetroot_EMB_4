#include <Arduino.h>
#include <atomic>
#include <OneButton.h>

#define BTN_STOP_ALARM 15

std::atomic<uint32_t> isrCounter(0);
volatile bool timerFired = false;
hw_timer_t *timer = nullptr;
bool timerActive = false;

void ARDUINO_ISR_ATTR onTimer() {
	isrCounter.fetch_add(1, std::memory_order_relaxed);
	timerFired = true;
}

OneButton button(BTN_STOP_ALARM, true, true);

void startTimer() {
	if (!timerActive) {
		timer = timerBegin(0, 80, true);
		timerAttachInterrupt(timer, &onTimer, true);
		timerAlarmWrite(timer, 1000000, true);
		timerAlarmEnable(timer);
		timerActive = true;
		Serial.println("Timer started...");
	}
}

void stopTimer() {
	if (timerActive && timer) {
		timerDetachInterrupt(timer);
		timerEnd(timer);
		timer = nullptr;
		timerActive = false;
		Serial.println("!!! Timer stopped by user !!!");
	}
}

void toggleTimer() {
	if (timerActive) {
		stopTimer();
	} else {
		startTimer();
	}
}

void setup() {
	Serial.begin(115200);
	button.attachClick(toggleTimer);
	startTimer();
}

void loop() {
	if (timerFired) {
		timerFired = false;
		uint32_t currentCount = isrCounter.load();
		Serial.print("Trigger #: ");
		Serial.print(currentCount);
		Serial.print(" | Time: ");
		Serial.print(millis());
		Serial.println(" ms");
	}

	button.tick();

	delay(1);
}

