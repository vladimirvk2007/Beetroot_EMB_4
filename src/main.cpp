#include <Arduino.h>
#include "maximilian.h"
#include "audio.h"

// Визначення піни для I2S інтерфейсу
#define I2S_BCK_PIN 15   // Пін бітового тактування (Bit Clock)
#define I2S_WS_PIN 17    // Пін вибору слова (Word Select / LRCLK)
#define I2S_DATA_PIN 16  // Пін передачі аудіоданих (Data Out)

// Осцилятор для генерації тону
static maxiOsc osc;

// Констани для конфігурації генератора тонів
const uint32_t SAMPLE_RATE = 32000;              // Частота дискретизації: 32 kHz
const uint32_t TONE_DURATION_MS = 1000;         // Тривалість одного тону: 1 сек
const uint32_t SAMPLES_PER_TONE = SAMPLE_RATE * TONE_DURATION_MS / 1000; // Кількість семплів на один тон
const int MIN_FREQUENCY = 200;                  // Мінімальна частота: 200 Гц
const int MAX_FREQUENCY = 3000;                 // Максимальна частота: 3000 Гц
const int FREQUENCY_STEP = 200;                 // Крок збільшення частоти: 200 Гц

// Змінні стану генератора
static int currentFrequency = MIN_FREQUENCY;    // Поточна частота
static int frequencyStep = FREQUENCY_STEP;      // Напрямок змінення частоти (+/-)
static uint32_t sampleCount = 0;                // Лічильник семплів для поточного тону
static bool audioReady = false;                 // Флаг готовності аудіомодуля

// Функція генерування аудіо сигналу, що викликається для кожного буфера
// Генерує синусоїдальні хвилі з повільно змінюється частотою
void play(float *output) {
  // Однаковий сигнал в обох каналах; рівень гучності — 30%
  output[0] = osc.sinewave(currentFrequency) * 0.3;
  output[1] = output[0];

  // Лічильник семплів для визначення коли змінити частоту
  if (++sampleCount >= SAMPLES_PER_TONE) {
    sampleCount = 0;
    currentFrequency += frequencyStep;

    // Перевірка меж і розворот напрямку при досягненні максимуму
    if (currentFrequency >= MAX_FREQUENCY) {
      currentFrequency = MAX_FREQUENCY;
      frequencyStep = -FREQUENCY_STEP;
    }
    // Розворот напрямку при досягненні мінімуму
    else if (currentFrequency <= MIN_FREQUENCY) {
      currentFrequency = MIN_FREQUENCY;
      frequencyStep = FREQUENCY_STEP;
    }
  }
}

// Ініціалізація плати та аудіомодуля
void setup() {
  // Запуск серійного з'єднання для відлагодження та логування
  Serial.begin(115200);
  Serial.println("Starting tone generator...");

  // Конфігурація для аудіомодуля
  const audio_config_t config = {
    I2S_BCK_PIN,         // Пін BCK
    I2S_WS_PIN,          // Пін WS (LRCLK)
    I2S_DATA_PIN,        // Пін DATA (DIN)
    SAMPLE_RATE,         // Частота дискретизації 32 kHz
    256,                 // Розмір буфера в семплах
    play                 // Функція генерування аудіо
  };

  // Ініціалізація аудіомодуля
  audioReady = audio_init(config);
  if (audioReady) {
    Serial.println("Generating tones from 200 to 3000 Hz");
  }
}

void loop() {
  // Генерація та передача наступного аудіобуфера
  if (audioReady) {
    audio_run();
  }
}
