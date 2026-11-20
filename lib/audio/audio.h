#ifndef AUDIO_H
#define AUDIO_H

#include <stddef.h>
#include <stdint.h>

// Початкові параметри I2S; декодер може змінити формат відповідно до MP3.
typedef struct {
  int pinBck;
  int pinWs;
  int pinData;
  int sampleRate;
  int bufferSize;
} audio_config_t;

// Ініціалізує I2S і декодер; повертає false у разі помилки.
bool audio_mp3_init(const audio_config_t &config);
// Скидає стан декодера перед підключенням нового потоку.
bool audio_mp3_reset();
// Подає MP3-дані та повертає кількість оброблених вхідних байтів.
size_t audio_mp3_write(const uint8_t *data, size_t size);

#endif // AUDIO_H
