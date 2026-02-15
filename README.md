**ESP32 WS2812 Rainbow Demo**

[WS2812 Datasheet](https://github.com/littlebirdelectronics/LB-00130/blob/master/datasheets/WS2812.pdf)

- **Overview:** Simple demo that runs a rainbow cycle on a single WS2812 (NeoPixel) LED connected to an ESP32 board.

- **Source:** src/main.cpp

**Hardware**
- **Board:** YD-ESP32-S3 (ESP32-S3 N16R8)
- **LED:** WS2812 / NeoPixel (single RGB LED)
- **Data pin:** Default set to `LED_PIN = 48` in `src/main.cpp`.

**Features**
- Smooth rainbow animation using the Adafruit_NeoPixel library.
- Configurable brightness and number of pixels via `src/main.cpp`.

**Build & Flash (PlatformIO)**

```bash
pio run
pio run -t upload
```


