#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN     48  // YD-ESP32-S3 onboard WS2812 (IO48)
#define NUMPIXELS   1   // One RGB LED only

Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
    Serial.begin(115200);
    pixels.begin();
    pixels.setBrightness(100); // Brightness (0~255)
    Serial.println("Rainbow demo start with brightness 100");
}

uint32_t Wheel(byte pos) {
    pos = 255 - pos;
    if(pos < 85) {
        return pixels.Color(255 - pos * 3, 0, pos * 3);
    } else if(pos < 170) {
        pos -= 85;
        return pixels.Color(0, pos * 3, 255 - pos * 3);
    } else {
        pos -= 170;
        return pixels.Color(pos * 3, 255 - pos * 3, 0);
    }
}

void loop() {
    for(int i = 0; i < 256; i++) {
        pixels.setPixelColor(0, Wheel(i));
        pixels.show();
        delay(20);
    }
}
