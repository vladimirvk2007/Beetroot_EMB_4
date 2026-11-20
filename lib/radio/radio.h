#ifndef RADIO_H
#define RADIO_H

#include "audio.h"

typedef struct {
  const char *url;
  audio_config_t audio;
} radio_config_t;

bool radio_init(const radio_config_t &config);
void radio_run(bool wifiConnected);

#endif // RADIO_H
