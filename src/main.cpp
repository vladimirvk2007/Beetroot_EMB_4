#include <Arduino.h>
#include <atomic>

#define BTN_STOP_ALARM 15

std::atomic<uint32_t> isrCounter(0);
volatile bool timerFired = false;

hw_timer_t *timer = NULL;

// Функція переривання (ISR)
void ARDUINO_ISR_ATTR onTimer() {
	// Операції ++ та присвоєння для atomic є безпечними (атомарними)
	isrCounter++;
	timerFired = true;
}

void setup() {
	Serial.begin(115200);

	pinMode(BTN_STOP_ALARM, INPUT_PULLUP);

	// Ініціалізація таймера (ESP32-S3)
	// divider = 80: 80 МГц / 80 = 1 МГц (1 tick = 1 мкс)
	timer = timerBegin(0, 80, true);

	// Прив'язка функції переривання
	timerAttachInterrupt(timer, &onTimer, true);

	// Налаштування спрацювання: 1 000 000 мікросекунд = 1 секунда
	timerAlarmWrite(timer, 1000000, true);
	timerAlarmEnable(timer);

	Serial.println("Timer started...");
}

void loop() {
	if (timerFired) {
		timerFired = false;

		// Safely read the counter value
		uint32_t currentCount = isrCounter.load();

		Serial.print("Trigger #: ");
		Serial.print(currentCount);
		Serial.print(" | Time: ");
		Serial.print(millis());
		Serial.println(" ms");
	}

	// Перевірка натискання кнопки для зупинки
	if (digitalRead(BTN_STOP_ALARM) == LOW) {
		if (timer) {
			timerEnd(timer);
			timer = NULL;
			Serial.println("!!! Timer stopped by user !!!");
		}
	}

	delay(10);
}

