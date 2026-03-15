#include "ButtonFSM.h"

int Button_FSM_Init(Button_FSM_t *fsm, uint8_t pin, uint32_t debounceTimeMs,
                        ButtonCallback_t cb, void* arg) {
    if (fsm == NULL) {
        Serial.println("Error: FSM pointer is NULL");
        return -1;
    }

    fsm->pin = pin;
    fsm->state = BUTTON_STATE_IDLE;
    fsm->debounceTime = debounceTimeMs;
    fsm->lastChangeTime = 0;
    fsm->callback = cb;
    fsm->arg = arg;
    return 0;
}

int Button_FSM_Update(Button_FSM_t *fsm) {
    int pinState = 0;
    uint32_t now = 0;

    if (fsm == NULL) {
        Serial.println("Error: FSM pointer is NULL");
        return -1;
    }

    pinState = digitalRead(fsm->pin);
    now = millis();

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

    return 0;
}

int Button_FSM_If_Pressed(Button_FSM_t *fsm, bool *fsm_state) {
    if (fsm == NULL || fsm_state == NULL) {
        Serial.println("Error: FSM pointer or state pointer is NULL");
        return -1;
    }

    if (fsm->state == BUTTON_STATE_PRESSED) {
        *fsm_state = true;
        return 0;
    }

    *fsm_state = false;

    return 0;
}
