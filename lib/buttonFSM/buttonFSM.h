#ifndef BUTTON_FSM_H
#define BUTTON_FSM_H

#include <Arduino.h>

typedef enum {
	BUTTON_STATE_IDLE = 0,
	BUTTON_STATE_PRESSED
} ButtonState_t;


typedef void (*ButtonCallback_t)(void* arg);

typedef struct {
	uint8_t pin;
	ButtonState_t state;
	uint32_t debounceTime;
	uint32_t lastChangeTime;
	ButtonCallback_t pressedCb;
	ButtonCallback_t releasedCb;
	void* arg;
} Button_FSM_t;

int Button_FSM_Init(Button_FSM_t *fsm, uint8_t pin, uint32_t debounceTimeMs,
                        ButtonCallback_t pressedCb, ButtonCallback_t releasedCb, void* arg);
int Button_FSM_Deinit(Button_FSM_t *fsm);

#endif // BUTTON_FSM_H
