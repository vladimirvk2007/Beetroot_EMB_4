#include "buttonFSM.h"

static void IRAM_ATTR buttonFSM(Button_FSM_t *fsm) {
    if (fsm == NULL) {
        return;
    }

    uint32_t now = millis();
    if (now - fsm->lastChangeTime < fsm->debounceTime) {
        return;
    }

    switch (fsm->state) {
        case BUTTON_STATE_IDLE:
            if (digitalRead(fsm->pin) == LOW) {
                fsm->lastChangeTime = now;
                fsm->state = BUTTON_STATE_PRESSED;
                if (fsm->pressedCb != NULL) {
                    fsm->pressedCb(fsm->arg);
                }
            }
            break;
        case BUTTON_STATE_PRESSED:
            if (digitalRead(fsm->pin) == HIGH) {
                fsm->lastChangeTime = now;
                fsm->state = BUTTON_STATE_IDLE;
                if (fsm->releasedCb != NULL) {
                    fsm->releasedCb(fsm->arg);
                }
            }
            break;
    }
}

static void IRAM_ATTR buttonInterruptHandler(void *argument) {
    buttonFSM((Button_FSM_t *)argument);
}

int Button_FSM_Init(Button_FSM_t *fsm, uint8_t pin, uint32_t debounceTimeMs,
                       ButtonCallback_t pressedCb, ButtonCallback_t releasedCb, void* arg) {
    if (fsm == NULL) {
        Serial.println("Error: FSM pointer is NULL");
        return -1;
    }

    fsm->pin = pin;
    fsm->state = BUTTON_STATE_IDLE;
    fsm->debounceTime = debounceTimeMs;
    fsm->lastChangeTime = 0;
    fsm->pressedCb = pressedCb;
    fsm->releasedCb = releasedCb;
    fsm->arg = arg;
    pinMode(pin, INPUT_PULLUP);
	attachInterruptArg(digitalPinToInterrupt(pin), buttonInterruptHandler, fsm, CHANGE);
    return 0;
}

int Button_FSM_Deinit(Button_FSM_t *fsm) {
    if (fsm == NULL) {
        Serial.println("Error: FSM pointer or state pointer is NULL");
        return -1;
    }

    detachInterrupt(digitalPinToInterrupt(fsm->pin));

    return 0;
}
