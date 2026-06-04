
# ESP32 Button FSM Example

This project demonstrates a small finite state machine for button debouncing on an ESP32-S3 board using the Arduino framework.

The active example in `src/main.cpp` reads a button on GPIO 15 with the internal pull-up enabled, filters bounce with a timed FSM, counts valid presses, prints the counter to Serial, and toggles an LED on GPIO 16.

## Features

- FSM-based debounce with explicit intermediate states
- Valid press counter
- Serial output at 115200 baud
- LED toggle on confirmed button press

## Hardware

- **Board:** ESP32-S3
- **Button:** GPIO 15, wired to GND, using `INPUT_PULLUP`
- **LED output:** GPIO 16

## Project Layout

- `src/main.cpp` - Application entry point and button FSM example
- `src/efsm.h` - Small macro-based FSM helper
- `lib/buttonFSM/` - Older standalone button FSM implementation kept in the repo
- `platformio.ini` - PlatformIO configuration

## How It Works

1. The loop reads the physical button level.
2. If the FSM is in a transition state, it waits for `DEBOUNCE_DELAY` before accepting the change.
3. A stable press calls `onButtonPress()`, which increments the counter and toggles the LED.
4. Release uses the same debounce pattern before returning to the idle state.

## Build and Upload

```bash
# Build project
pio run

# Upload firmware
pio run -t upload

# Open serial monitor
pio device monitor -b 115200

# Upload and monitor in one step
pio run -t upload && pio device monitor -b 115200
```

## Serial Output Example

```text
Button pressed #1
Toggling LED.
Button pressed #2
Toggling LED.
Button pressed #3
Toggling LED.
```

## Notes

- `src/main.cpp` is the active implementation.
- The `lib/buttonFSM/` code is preserved for reference, but it is not used by the current sketch.

