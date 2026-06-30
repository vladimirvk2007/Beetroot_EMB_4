#include "buttonFSM.h"

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
                Serial.println("Button pressed, entering Debounce State");
                fsm->lastChangeTime = now;
            }
            break;
        case BUTTON_STATE_DEBOUNCE:
            if ((now - fsm->lastChangeTime) >= fsm->debounceTime) {
                if (pinState == LOW) {
                    fsm->state = BUTTON_STATE_PRESSED;
                    Serial.println("Entering Pressed State");
                    if (fsm->pressedCb) {
                        fsm->pressedCb(fsm->arg);
                    }
                } else {
                    fsm->state = BUTTON_STATE_IDLE;
                    Serial.println("Button released, returning to Idle State");
                    if (fsm->releasedCb) {
                        fsm->releasedCb(fsm->arg);
                    }
                }
            }
            break;
        case BUTTON_STATE_PRESSED:
            if (pinState == HIGH) {
                fsm->state = BUTTON_STATE_IDLE;
                Serial.println("Button released, returning to Idle State");
                if (fsm->releasedCb) {
                    fsm->releasedCb(fsm->arg);
                }
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
