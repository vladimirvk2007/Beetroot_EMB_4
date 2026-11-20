#ifndef RADIO_H
#define RADIO_H

#include "audio.h"

typedef struct {
  // Рядок URL має залишатися доступним протягом роботи модуля.
  const char *url;
  audio_config_t audio;
} radio_config_t;

// Ініціалізує аудіо та зберігає конфігурацію радіо.
bool radio_init(const radio_config_t &config);
// Обробляє потік; wifiConnected вказує поточний стан Wi-Fi.
void radio_run(bool wifiConnected);

#endif // RADIO_H
