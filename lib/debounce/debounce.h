#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <stdint.h>

class Debounce {
public:
    Debounce(bool initialState, uint32_t debounceMs);

    // Returns true only when a new stable state is accepted.
    bool update(bool rawState, uint32_t nowMs);
    bool state() const;

private:
    bool stableState_;
    bool lastRawState_;
    uint32_t debounceMs_;
    uint32_t lastChangeMs_;
};

#endif // DEBOUNCE_H
