#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

// Функція заповнює два канали значеннями від -1.0 до 1.0.
typedef void (*audio_callback_t)(float *output);

typedef struct {
  int pinBck;
  int pinWs;
  int pinData;
  int sampleRate;
  int bufferSize;
  audio_callback_t callback;
} audio_config_t;

bool audio_init(const audio_config_t &config);
void audio_run();

#endif // AUDIO_H
