#include <Arduino.h>
#include "radio.h"
#include "wifi_module.h"

#define I2S_BCK_PIN 15
#define I2S_WS_PIN 17
#define I2S_DATA_PIN 16

// Облікові дані зберігаються в локальному файлі, який не відстежує Git.
#if __has_include("radio_config.h")
#include "radio_config.h"
#else
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#endif

const char *RADIO_URL = "http://stream.live.vc.bbcmedia.co.uk/bbc_world_service";
/* http://stream.live.vc.bbcmedia.co.uk/bbc_world_service */
/* http://icepe6.infomaniak.ch:80/jazz-wr01-128.mp3 */
static bool ready = false;

// Ініціалізує послідовний порт, Wi-Fi та радіо.
void setup() {
  Serial.begin(115200);

  // Початковий формат I2S буде оновлено за параметрами MP3-потоку.
  const radio_config_t config = {
    RADIO_URL,
    {I2S_BCK_PIN, I2S_WS_PIN, I2S_DATA_PIN, 44100, 256}
  };
  if (!wifi_init(WIFI_SSID, WIFI_PASSWORD)) {
    return;
  }
  ready = radio_init(config);
}

// Регулярно обробляє Wi-Fi-з'єднання та радіопотік.
void loop() {
  if (ready) {
    // Радіо використовує поточний стан Wi-Fi для керування потоком.
    radio_run(wifi_run());
  } else {
    delay(1000);
  }
}
