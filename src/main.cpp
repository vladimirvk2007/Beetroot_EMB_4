#include "AudioTools.h"
#include "AudioTools/AudioLibs/MaximilianDSP.h"

#define I2S_BCK_PIN 15
#define I2S_WS_PIN 17
#define I2S_DATA_PIN 16

// Аудіовихід через I2S
I2SStream out;
Maximilian maximilian(out);

// Осцилятор для генерації тону
maxiOsc osc;

// Фільтр низьких частот
maxiFilter filter;

// Частота тону (440 Гц — нота ля)
const double TONE_FREQUENCY = 440.0;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting tone generator...");

  // Налаштування аудіовиходу
  auto cfg = out.defaultConfig(TX_MODE);
  cfg.is_master = true;  // ESP32 генерує тактовий сигнал
  cfg.pin_bck = I2S_BCK_PIN;
  cfg.pin_ws = I2S_WS_PIN;
  cfg.pin_data = I2S_DATA_PIN;
  cfg.sample_rate = 32000;
  cfg.channels = 2;
  cfg.bits_per_sample = 16;
  cfg.buffer_size = 256;

  out.begin(cfg);
  maximilian.begin(cfg);

  Serial.println("Generating 440 Hz tone...");
}

void play(float *output) {
  // Генерація синусоїди із заданою частотою
  double tone = osc.sinewave(TONE_FREQUENCY);

  // Фільтрація високих частот
  double filtered = filter.lores(tone, 2000, 0.5);

  // Однаковий сигнал в обох каналах; рівень гучності — 30%
  output[0] = filtered * 0.3;
  output[1] = output[0];
}

void loop() {
  // Генерація та передача наступного аудіобуфера
  maximilian.copy();
}
