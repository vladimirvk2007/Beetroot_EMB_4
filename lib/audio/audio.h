#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

// Функція заповнює два канали значеннями від -1.0 до 1.0.
// Параметр output містить два значення [лівий канал, правий канал]
typedef void (*audio_callback_t)(float *output);

// Структура конфігурації для ініціалізації аудіомодуля
typedef struct {
  int pinBck;              // Пін для сигналу бітового тактування (BCK)
  int pinWs;               // Пін для сигналу вибору слова (WS/LRCLK)
  int pinData;             // Пін для передачі аудіоданих (DIN)
  int sampleRate;          // Частота дискретизації (Hz)
  int bufferSize;          // Розмір буфера в семплах
  audio_callback_t callback; // Функція зворотного виклику для генерування аудіо
} audio_config_t;

// Ініціалізація аудіомодуля з заданою конфігурацією
// Повертає true при успіху, false при помилці
bool audio_init(const audio_config_t &config);

// Запуск обробки наступного аудіобуфера
// Має викликатись регулярно в main loop
void audio_run();

#endif // AUDIO_H
