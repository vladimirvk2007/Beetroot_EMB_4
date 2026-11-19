#include <Arduino.h>
#include "maximilian.h"
#include "audio.h"

#define I2S_BCK_PIN 15
#define I2S_WS_PIN 17
#define I2S_DATA_PIN 16

// Осцилятор для генерації тону
static maxiOsc osc;

// Фільтр низьких частот
static maxiFilter filter;

// Частота тону (440 Гц — нота ля)
const double TONE_FREQUENCY = 440.0;
static bool audioReady = false;

void play(float *output) {
  // Генерація синусоїди із заданою частотою
  double tone = osc.sinewave(TONE_FREQUENCY);

  // Фільтрація високих частот
  double filtered = filter.lores(tone, 2000, 0.5);

  // Однаковий сигнал в обох каналах; рівень гучності — 30%
  output[0] = filtered * 0.3;
  output[1] = output[0];
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting tone generator...");

  const audio_config_t config = {
    I2S_BCK_PIN,
    I2S_WS_PIN,
    I2S_DATA_PIN,
    32000,
    256,
    play
  };
  audioReady = audio_init(config);
  if (audioReady) {
    Serial.println("Generating 440 Hz tone...");
  }
}

void loop() {
  // Генерація та передача наступного аудіобуфера
  if (audioReady) {
    audio_run();
  }
}
