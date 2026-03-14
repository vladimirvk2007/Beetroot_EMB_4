#include "ButtonFSM.h"

void Button_FSM_Init(Button_FSM_t *fsm, uint8_t pin, uint32_t debounceTimeMs,
                        ButtonCallback_t cb, void* arg) {
    fsm->pin = pin;
    fsm->state = BUTTON_STATE_IDLE;
    fsm->debounceTime = debounceTimeMs;
    fsm->lastChangeTime = 0;
    fsm->callback = cb;
    fsm->arg = arg;
}

void Button_FSM_Update(Button_FSM_t *fsm) {
    int pinState = digitalRead(fsm->pin);
    uint32_t now = millis();

    switch (fsm->state) {
        case BUTTON_STATE_IDLE:
            if (pinState == LOW) {
                fsm->state = BUTTON_STATE_DEBOUNCE;
                fsm->lastChangeTime = now;
            }
            break;
        case BUTTON_STATE_DEBOUNCE:
            if ((now - fsm->lastChangeTime) >= fsm->debounceTime) {
                if (pinState == LOW) {
                    fsm->state = BUTTON_STATE_PRESSED;
                    if (fsm->callback) {
                        fsm->callback(fsm->arg);
                    }
                } else {
                    fsm->state = BUTTON_STATE_IDLE;
                }
            }
            break;
        case BUTTON_STATE_PRESSED:
            if (pinState == HIGH) {
                fsm->state = BUTTON_STATE_IDLE;
            }
            break;
    }
}

bool Button_FSM_If_Pressed(Button_FSM_t *fsm) {
    if (fsm->state == BUTTON_STATE_PRESSED) {
        return true;
    }
    return false;
}
