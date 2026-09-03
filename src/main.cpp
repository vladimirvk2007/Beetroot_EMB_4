#include <Arduino.h>
#include <atomic>
#include <OneButton.h>
#include <IWatchdog.h>

#define BTN_START_TIMER_IN PA0
#define TIMER_LED_OUT PC13
#define WATCHDOG_TIMEOUT_US 10000000

std::atomic<uint32_t> isrCounter(0);
volatile bool timerFired = false;

HardwareTimer *timer = NULL;
OneButton startButton(BTN_START_TIMER_IN, true, true);
uint32_t timeDuration = 0;

// Функція переривання (ISR)
void onTimer() {
	// Операції ++ та присвоєння для atomic є безпечними (атомарними)
	isrCounter.fetch_add(1, std::memory_order_relaxed);
	timerFired = true;
	timer->pause();
}

void startTimer() {
	digitalWrite(TIMER_LED_OUT, HIGH);
	// Старт одноразового таймера на 1 секунду.
	timer->setCount(0);
	timer->resume();
	// Запис поточного часу
	timeDuration = millis();
}

void setup() {
	Serial.begin(115200);
	IWatchdog.begin(WATCHDOG_TIMEOUT_US);

	pinMode(TIMER_LED_OUT, OUTPUT);
	digitalWrite(TIMER_LED_OUT, LOW);

	startButton.attachPress(startTimer);

	// Ініціалізація апаратного таймера STM32: 1 MHz, переривання через 1 с.
	timer = new HardwareTimer(TIM2);
	timer->setOverflow(1000000, MICROSEC_FORMAT);
	timer->attachInterrupt(onTimer);
	timer->pause();

	Serial.println("Press the button to start the timer...");
}

void loop() {
	startButton.tick();

	if (timerFired) {
		timerFired = false;
		digitalWrite(TIMER_LED_OUT, LOW);
		const uint32_t elapsedTime = millis() - timeDuration;
		Serial.printf("Timer finished %u times. Elapsed time: %lu ms\n",
			isrCounter.load(std::memory_order_relaxed), elapsedTime);

        IWatchdog.reload();
	}
}

