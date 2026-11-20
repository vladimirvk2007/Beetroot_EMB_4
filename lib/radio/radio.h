#ifndef RADIO_H
#define RADIO_H

#include "audio.h"

typedef struct {
  const char *ssid;
  const char *password;
  const char *url;
  audio_config_t audio;
} radio_config_t;

bool radio_init(const radio_config_t &config);
void radio_run();

#endif // RADIO_H
