# ESP32-S3 tone generator

The sketch generates a continuous 440 Hz sine tone without buttons. It sends
16-bit stereo audio at 32 kHz to an external I2S DAC or amplifier:

- BCK: GPIO 15
- WS/LRCK: GPIO 17
- DATA: GPIO 16

Connect the device's ground to the ESP32-S3 ground and power it according to
its specifications. Adjust `TONE_FREQUENCY` in `src/main.cpp` to change the
tone.

PlatformIO installs AudioTools and Maximilian from the versions specified in
`platformio.ini` when building:

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```
