#ifndef BUTTON_FSM_H
#define BUTTON_FSM_H

#include <Arduino.h>

typedef enum {
	BUTTON_STATE_IDLE = 0,
	BUTTON_STATE_DEBOUNCE,
	BUTTON_STATE_PRESSED
} ButtonState_t;

typedef struct {
	uint8_t pin;
	ButtonState_t state;
	uint32_t debounceTime;
	uint32_t lastChangeTime;
} Button_FSM_t;

void Button_FSM_Init(Button_FSM_t *fsm, uint8_t pin, uint32_t debounceTimeMs);
void Button_FSM_Update(Button_FSM_t *fsm);
bool Button_FSM_If_Pressed(Button_FSM_t *fsm);

#endif // BUTTON_FSM_H
