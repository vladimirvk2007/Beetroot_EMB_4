#include <Arduino.h>
#include <atomic>
#include <OneButton.h>
#include <esp_task_wdt.h>

#define BTN_START_TIMER_IN 15
#define TIMER_LED_OUT 16
#define WDT_TIMEOUT_S 10

std::atomic<uint32_t> isrCounter(0);
volatile bool timerFired = false;

hw_timer_t *timer = NULL;
OneButton startButton(BTN_START_TIMER_IN, true, true);
uint32_t timeDuration = 0;

// Функція переривання (ISR)
void IRAM_ATTR onTimer() {
	// Операції ++ та присвоєння для atomic є безпечними (атомарними)
	isrCounter.fetch_add(1, std::memory_order_relaxed);
	timerFired = true;
}

void startTimer() {
	digitalWrite(TIMER_LED_OUT, HIGH);
	// Старт таймера
	timerWrite(timer, 0);
	timerAlarmWrite(timer, 1000000, false);
	timerAlarmEnable(timer);
	// Запис поточного часу
	timeDuration = millis();
}

void setup() {
	Serial.begin(115200);
	//enableLoopWDT();

	pinMode(TIMER_LED_OUT, OUTPUT);
	digitalWrite(TIMER_LED_OUT, LOW);

	startButton.attachPress(startTimer);

	// Ініціалізація таймера (ESP32-S3)
	// divider = 80: 80 МГц / 80 = 1 МГц (1 tick = 1 мкс)
	timer = timerBegin(0, 80, true);

	// Прив'язка функції переривання
	timerAttachInterrupt(timer, &onTimer, true);

	// Ініціалізація Watchdog Timer (WDT)
	esp_task_wdt_init(WDT_TIMEOUT_S, false); // true - reset
	esp_task_wdt_add(NULL);

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

		esp_task_wdt_reset();
	}
}

