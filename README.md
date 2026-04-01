
# ESP32-S3 PWM Sine Wave Tone Generator (ESP-IDF, PlatformIO)

This project demonstrates:
- Generating musical notes using PWM (LEDC) on GPIO18
- Producing a sine wave using a lookup table for smooth sound
- Playing a sequence of notes (C4, D4, E4, F4, G4, A4, B4, C5) in a loop
- Modern ESP-IDF (PlatformIO) code style and compatibility

## Main Features
- Generates a sine wave using 12-bit PWM (LEDC) on GPIO18
- Plays a predefined melody by cycling through musical notes
- Each note is played for 1 second with a short pause between notes
- Uses a 64-point sine table for smooth waveform output
- All timing and waveform generation is handled in software

### Main Code
- File: `src/main.cpp`
- PWM: GPIO18, 12-bit, ~18 kHz carrier, LEDC (low speed mode)
- Sine table: 64 samples, 12-bit resolution

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
- Uses a 64-point sine lookup table for smooth PWM audio
- PWM (LEDC) is configured for 12-bit resolution and ~18 kHz frequency
- Musical notes are defined as frequencies and played in sequence
- All critical ESP-IDF calls are wrapped with ESP_ERROR_CHECK for diagnostics

## Configuration
- To change the PWM output pin, modify `PWM_GPIO` in `main.cpp`
- To change the melody or note durations, edit the `notes` array and timing in `main.cpp`

## More Info
- ESP-IDF LEDC (PWM) documentation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/ledc.html

