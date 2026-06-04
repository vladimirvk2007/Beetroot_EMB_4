#ifndef EASY_FINITE_STATE_MACHINE_H
#define EASY_FINITE_STATE_MACHINE_H

typedef void (*FsmAction)(void);

#define DEFINE_STATES(...) enum States { __VA_ARGS__, NUM_STATES };
#define DEFINE_EVENTS(...) enum Events { __VA_ARGS__, NUM_EVENTS };

#define CREATE_FSM(initialState) \
    States CURRENT_STATE = initialState; \
    States nextState = initialState; \
    Events currentEvent; \
    bool eventTriggered = false;

#define TRIGGER_EVENT(event) \
    if (!eventTriggered) { \
        currentEvent = event; \
        eventTriggered = true; \
    }

#define BEGIN_TRANSITIONS \
    void updateFsmTransitions() { \
        if (!eventTriggered) return; \
        eventTriggered = false;

#define TRANSITION(fromState, event, toState, action) \
        if (CURRENT_STATE == fromState && currentEvent == event) { \
            nextState = toState; \
            FsmAction transitionAction = action; \
            if (transitionAction != NULL) transitionAction(); \
        }

#define END_TRANSITIONS \
    }

#define UPDATE_FSM() \
    updateFsmTransitions(); \
    CURRENT_STATE = nextState;

#endif // EASY_FINITE_STATE_MACHINE_H
