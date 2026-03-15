
# ESP32 Button FSM Example

This project demonstrates the use of a finite state machine (FSM) for button debouncing and event handling on an ESP32-S3 DevKitC-1 board using the Arduino framework. It also compares a simple interrupt-based button counter with the FSM approach.

## Features

- Button press detection using hardware interrupt
- Button debouncing and event handling using a custom FSM
- Two LEDs: one shows the raw button state, the other shows the FSM-processed state
- Serial output for button press counts (both raw and FSM)

## Hardware

- **Board:** ESP32-S3-DevKitC-1
- **Button:** Connected to GPIO 15 (with internal pull-up)
- **LEDs:**
  - Raw button state: GPIO 7
  - FSM output: GPIO 16

## File Structure

- `src/main.cpp` — Main application logic
- `lib/buttonFSM/` — Button FSM implementation
- `platformio.ini` — PlatformIO project configuration

## How It Works

1. **Interrupt-based Counter:**
	- An interrupt is attached to the button pin. Each falling edge increments a counter and sets a flag.
	- The main loop prints the count when the flag is set.

2. **FSM-based Debouncing:**
	- The FSM debounces the button and calls a callback on a valid press.
	- The main loop prints the FSM press count and updates the FSM LED.

## Build and Upload

```bash
# Build project
pio run

# Upload firmware to ESP32
pio run -t upload

# Monitor serial output at 115200 baud
pio device monitor -b 115200

# Combine upload and monitor
pio run -t upload && pio device monitor -b 115200
```

## Example Serial Output

```
Button pressed 1 times
Button pressed 2 times
FSM Button pressed 1 times
Button pressed 3 times
Button pressed 4 times
Button pressed 5 times
FSM Button pressed 2 times
Button pressed 6 times
FSM Button pressed 3 times
Button pressed 7 times
FSM Button pressed 4 times
Button pressed 8 times
Button pressed 9 times
FSM Button pressed 5 times
Button pressed 10 times
Button pressed 11 times
Button pressed 12 times
FSM Button pressed 6 times
Button pressed 13 times
Button pressed 14 times
FSM Button pressed 7 times
```

## Dependencies

No external libraries required. The FSM is implemented in `lib/buttonFSM/`.

