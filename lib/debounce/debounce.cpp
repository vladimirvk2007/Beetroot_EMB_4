#include "debounce.h"

Debounce::Debounce(bool initialState, uint32_t debounceMs)
    : stableState_(initialState),
      lastRawState_(initialState),
      debounceMs_(debounceMs),
      lastChangeMs_(0) {
}

bool Debounce::update(bool rawState, uint32_t nowMs) {
    if (rawState != lastRawState_) {
        lastRawState_ = rawState;
        lastChangeMs_ = nowMs;
    }

    if ((nowMs - lastChangeMs_) >= debounceMs_ && stableState_ != lastRawState_) {
        stableState_ = lastRawState_;
        return true;
    }

    return false;
}

bool Debounce::state() const {
    return stableState_;
}
