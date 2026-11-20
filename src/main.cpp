#include <Arduino.h>
#include "radio.h"
#include "wifi_module.h"

#define I2S_BCK_PIN 15
#define I2S_WS_PIN 17
#define I2S_DATA_PIN 16

#if __has_include("radio_config.h")
#include "radio_config.h"
#else
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#endif

const char *RADIO_URL = "http://icepe6.infomaniak.ch:80/jazz-wr01-128.mp3";
static bool ready = false;

void setup() {
  Serial.begin(115200);

  const radio_config_t config = {
    RADIO_URL,
    {I2S_BCK_PIN, I2S_WS_PIN, I2S_DATA_PIN, 44100, 256}
  };
  if (!wifi_init(WIFI_SSID, WIFI_PASSWORD)) {
    return;
  }
  ready = radio_init(config);
}

void loop() {
  if (ready) {
    radio_run(wifi_run());
  } else {
    delay(1000);
  }
}
