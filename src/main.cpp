#include <Arduino.h>
#include <atomic>
#include <OneButton.h>

#define BTN_START_TIMER_IN 15
#define TIMER_LED_OUT 16

std::atomic<uint32_t> isrCounter(0);
volatile bool timerFired = false;

hw_timer_t *timer = NULL;
OneButton startButton(BTN_START_TIMER_IN, true, true);
bool timerEnabled = false;

// Функція переривання (ISR)
void IRAM_ATTR onTimer() {
	// Операції ++ та присвоєння для atomic є безпечними (атомарними)
	isrCounter.fetch_add(1, std::memory_order_relaxed);
	timerFired = true;
}

void startTimer() {
	if (!timer) {
		return;
	}

	timerFired = false;
	timerAlarmEnable(timer);
	digitalWrite(TIMER_LED_OUT, HIGH);
	timerEnabled = true;
	Serial.println("Timer started...");
}

void setup() {
	Serial.begin(115200);
	enableLoopWDT();

	pinMode(TIMER_LED_OUT, OUTPUT);
	digitalWrite(TIMER_LED_OUT, LOW);
	startButton.attachClick(startTimer);

	// Ініціалізація таймера (ESP32-S3)
	// divider = 80: 80 МГц / 80 = 1 МГц (1 tick = 1 мкс)
	timer = timerBegin(0, 80, true);

	// Прив'язка функції переривання
	timerAttachInterrupt(timer, &onTimer, true);

	// Одноразове спрацювання через 1 000 000 мікросекунд = 1 секунду
	timerAlarmWrite(timer, 1000000, false);
	timerAlarmDisable(timer);

	Serial.println("Press the button to start the timer...");
}

void loop() {
	startButton.tick();

	if (timerFired) {
		timerFired = false;
		timerEnabled = false;
		digitalWrite(TIMER_LED_OUT, LOW);

		// Safely read the counter value
		uint32_t currentCount = isrCounter.load();

		Serial.print("Trigger #: ");
		Serial.println(currentCount);
		//Serial.print(" | Time: ");
		//Serial.print(millis());
		//Serial.println(" ms");
	}

	feedLoopWDT();
	delay(10);
}

