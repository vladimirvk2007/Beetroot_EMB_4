#include <Arduino.h>
#include "maximilian.h"
#include "audio.h"

#define I2S_BCK_PIN 15
#define I2S_WS_PIN 17
#define I2S_DATA_PIN 16

// Осцилятор для генерації тону
static maxiOsc osc;

const uint32_t SAMPLE_RATE = 32000;
const uint32_t TONE_DURATION_MS = 1000;
const uint32_t SAMPLES_PER_TONE = SAMPLE_RATE * TONE_DURATION_MS / 1000;
const int MIN_FREQUENCY = 200;
const int MAX_FREQUENCY = 3000;
const int FREQUENCY_STEP = 200;
static int currentFrequency = MIN_FREQUENCY;
static int frequencyStep = FREQUENCY_STEP;
static uint32_t sampleCount = 0;
static bool audioReady = false;

void play(float *output) {
  // Однаковий сигнал в обох каналах; рівень гучності — 30%
  output[0] = osc.sinewave(currentFrequency) * 0.3;
  output[1] = output[0];

  if (++sampleCount >= SAMPLES_PER_TONE) {
    sampleCount = 0;
    currentFrequency += frequencyStep;
    if (currentFrequency >= MAX_FREQUENCY) {
      currentFrequency = MAX_FREQUENCY;
      frequencyStep = -FREQUENCY_STEP;
    } else if (currentFrequency <= MIN_FREQUENCY) {
      currentFrequency = MIN_FREQUENCY;
      frequencyStep = FREQUENCY_STEP;
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting tone generator...");

  const audio_config_t config = {
    I2S_BCK_PIN,
    I2S_WS_PIN,
    I2S_DATA_PIN,
    SAMPLE_RATE,
    256,
    play
  };
  audioReady = audio_init(config);
  if (audioReady) {
    Serial.println("Генерація тонів від 200 до 3000 Гц");
  }
}

void loop() {
  // Генерація та передача наступного аудіобуфера
  if (audioReady) {
    audio_run();
  }
}
