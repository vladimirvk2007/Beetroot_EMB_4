#include <Arduino.h>

#include "ota.h"

void setup() {
    Serial.begin(115200);

    otaSetup();
}

void loop() {
    otaHandle();
}

