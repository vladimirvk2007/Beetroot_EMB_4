#include <Arduino.h>
#include <atomic>
#include "buttonFSM.h"
#include "efsm.h"

#define BUTTON_PIN 			15
#define LED_FSM_PIN 		16
#define DEBOUNCE_DELAY 		50

// 1. Стани автомата
DEFINE_STATES(
  ST_RELEASED,     // Кнопка відпущена (очікування)
  ST_MAYBE_PRESSED, // Спроба натискання (брязкіт?)
  ST_PRESSED,      // Кнопка стабільно натиснута
  ST_MAYBE_RELEASED// Спроба відпускання (брязкіт?)
)

// 2. Події (тригери)
DEFINE_EVENTS(
  EV_PIN_LOW,     // На піні LOW (кнопка натиснута, якщо підтяжка INPUT_PULLUP)
  EV_PIN_HIGH,    // На піні HIGH (кнопка відпущена)
  EV_TIMEOUT      // Стабільний час минув
)

CREATE_FSM(ST_RELEASED)
unsigned long debounceTimer = 0;
uint32_t pressCount = 0;

// 6. Функції дій
void resetTimer() {
  debounceTimer = millis(); // Скидання таймера при першому коливанні сигналу
}

void onButtonPress() {
	pressCount++;
	Serial.print("Button pressed #");
	Serial.println(pressCount);
	digitalWrite(LED_FSM_PIN, !digitalRead(LED_FSM_PIN));
}

// 3. Таблиця переходів
BEGIN_TRANSITIONS
	// Очікування натискання -> фіксація першої зміни
	TRANSITION(ST_RELEASED,       EV_PIN_LOW,   ST_MAYBE_PRESSED,  resetTimer)

	// Стабілізація натискання -> якщо за 50мс рівень не змінився, кнопка дійсно натиснута
	TRANSITION(ST_MAYBE_PRESSED,  EV_PIN_HIGH,  ST_RELEASED,       NULL)
	TRANSITION(ST_MAYBE_PRESSED,  EV_TIMEOUT,   ST_PRESSED,        onButtonPress)

	// Очікування відпускання -> фіксація першої зміни
	TRANSITION(ST_PRESSED,        EV_PIN_HIGH,  ST_MAYBE_RELEASED, resetTimer)

	// Стабілізація відпускання -> якщо за 50мс рівень не змінився, кнопку відпустили
	TRANSITION(ST_MAYBE_RELEASED, EV_PIN_LOW,   ST_PRESSED,        NULL)
	TRANSITION(ST_MAYBE_RELEASED, EV_TIMEOUT,   ST_RELEASED,       NULL)
END_TRANSITIONS


void setup() {
	Serial.begin(115200);
	pinMode(BUTTON_PIN, INPUT_PULLUP);
	pinMode(LED_FSM_PIN, OUTPUT);
	digitalWrite(LED_FSM_PIN, LOW);
}

void loop() {
	// 4. Опитування фізичного стану піна
	bool isPressed = (digitalRead(BUTTON_PIN) == LOW);

	// 5. Перевірка таймера стабілізації для проміжних станів
	if ((CURRENT_STATE == ST_MAYBE_PRESSED || CURRENT_STATE == ST_MAYBE_RELEASED)
		&& (millis() - debounceTimer >= DEBOUNCE_DELAY)) {
		TRIGGER_EVENT(EV_TIMEOUT);
	} else if (isPressed) {
		TRIGGER_EVENT(EV_PIN_LOW);
	} else {
		TRIGGER_EVENT(EV_PIN_HIGH);
	}

	UPDATE_FSM();
}
