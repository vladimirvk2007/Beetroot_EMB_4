#include <Arduino.h>

#include "ota.h"
#include "rgb.h"

void setup() {
    Serial.begin(115200);

    otaSetup();
    rgbSetup();
}

void loop() {
    otaHandle();
    rgbLoop();
}

