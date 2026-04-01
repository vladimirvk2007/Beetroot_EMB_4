
# ESP32-S3 PWM Sine Wave Tone Generator (ESP-IDF, PlatformIO)


This project demonstrates:
- Generating musical notes using PWM (LEDC) on GPIO18
- Producing a sine wave using a lookup table for smooth sound
- Playing a sequence of notes (C4, D4, E4, F4, G4, A4, B4, C5) in a loop
- Toggling an LED on GPIO4 after each full melody cycle (LED turns on/off alternately)
- Modern ESP-IDF (PlatformIO) code style and compatibility


## Main Features
- Sine wave generation using 12-bit PWM (LEDC) on GPIO18
- Plays a predefined melody (C4 to C5) with 1 second per note
- 64-point sine table for smooth audio output
- LED on GPIO4 toggles after each melody (can be onboard or external)
- All timing and waveform generation handled in software


### Main Code
- File: `src/main.cpp`
- PWM: GPIO18, 12-bit, ~18 kHz carrier, LEDC (low speed mode)
- Sine table: 64 samples, 12-bit resolution
- LED: GPIO4, toggled after each note (state changes every time a note is played)


## PlatformIO
- Platform: `espressif32`
- Board: `esp32-s3-devkitc-1` (or compatible ESP32-S3)
- Framework: `espidf`
- Monitor speed: 115200 baud

## How to Build and Flash
1. Connect your ESP32-S3 board to your computer
2. Open a terminal in the project root
3. Run:
   ```
   pio run -t upload
   ```
## Code Highlights
- 64-point sine lookup table for smooth PWM audio
- PWM (LEDC) configured for 12-bit resolution and ~18 kHz frequency
- LED on GPIO4 toggles after each melody (easy to observe program state)
- Musical notes are defined as frequencies and played in sequence
- All critical ESP-IDF calls are wrapped with ESP_ERROR_CHECK for diagnostics

## Configuration
- To change the PWM output pin, modify `PWM_GPIO` in `main.cpp`
- To change the LED pin, modify `LED_GPIO` in `main.cpp`
- To change the melody or note durations, edit the `notes` array and timing in `main.cpp`

## More Info
- ESP-IDF LEDC (PWM) documentation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/ledc.html

