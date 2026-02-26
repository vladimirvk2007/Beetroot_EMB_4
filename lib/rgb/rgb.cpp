#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "rgb.h"

#define LED_PIN             48
#define NUMPIXELS           1
#define BLINK_INTERVAL_MS   1000L
//#define BLINK_INTERVAL_MS 200L

uint8_t rgb_state = 0;
unsigned long last_rgb_change = 0;

// --- WS2812 RGB LED ---
Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// --- Function to set WS2812 RGB LED color ---
void setRGB(uint8_t r, uint8_t g, uint8_t b) {
    pixels.setPixelColor(0, pixels.Color(r, g, b));
    pixels.show();
}

void rgbSetup() {
    pixels.begin();
    setRGB(0, 0, 0); // Start with LED off
}

void rgbLoop() {
    unsigned long now = millis();
    unsigned long elapsed = now - last_rgb_change;

    if (elapsed > BLINK_INTERVAL_MS) {
        last_rgb_change = now;
        rgb_state = (rgb_state + 1) % 3;
        switch (rgb_state) {
            case 0: setRGB(255, 0, 0); break; // Red
            case 1: setRGB(0, 255, 0); break; // Green
            case 2: setRGB(0, 0, 255); break; // Blue
        }
    }
}
