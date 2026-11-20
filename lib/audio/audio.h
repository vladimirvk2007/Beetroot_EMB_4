#ifndef AUDIO_H
#define AUDIO_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  int pinBck;
  int pinWs;
  int pinData;
  int sampleRate;
  int bufferSize;
} audio_config_t;

bool audio_mp3_init(const audio_config_t &config);
bool audio_mp3_reset();
size_t audio_mp3_write(const uint8_t *data, size_t size);

#endif // AUDIO_H
