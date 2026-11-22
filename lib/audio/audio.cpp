#include "audio.h"

#include "AudioTools.h"
#include "AudioTools/AudioLibs/MaximilianDSP.h"

namespace {
// I2S потік для передачі аудіо
I2SStream out;
// Функція зворотного виклику для генерування аудіо
audio_callback_t audioCallback = nullptr;
// Флаг ініціалізації модуля
bool initialized = false;

// Функція обробника аудіо, що викликається Maximilian
// Передає керування до користувацької функції зворотного виклику
void renderAudio(float *output) {
  audioCallback(output);
}

// Екземпляр обробника аудіо з бібліотеки Maximilian
Maximilian maximilian(out, DEFAULT_BUFFER_SIZE, renderAudio);
}

// Ініціалізація аудіомодуля з заданою конфігурацією
// Перевіряє валідність параметрів, налаштовує I2S, та запускає обробку аудіо
bool audio_init(const audio_config_t &config) {
  // Перевірка: модуль не повинен бути ініціалізований повторно
  if (initialized) {
    Serial.println("Error audio: module already initialized");
    return false;
  }
  // Перевірка валідності конфігурації: callback, частота, розмір буфера та піни
  if (config.callback == nullptr || config.sampleRate <= 0 ||
      config.bufferSize <= 0 || config.pinBck < 0 || config.pinWs < 0 ||
      config.pinData < 0 || config.pinBck == config.pinWs ||
      config.pinBck == config.pinData || config.pinWs == config.pinData) {
    Serial.println("Error audio: invalid configuration");
    return false;
  }

  // Налаштування I2S конфігурації в режимі передачі (TX_MODE)
  auto cfg = out.defaultConfig(TX_MODE);
  cfg.is_master = true;                    // Плата-master генерує сигнали синхронізації
  cfg.pin_bck = config.pinBck;             // Пін бітового тактування
  cfg.pin_ws = config.pinWs;               // Пін вибору слова (LRCLK)
  cfg.pin_data = config.pinData;           // Пін передачі даних
  cfg.sample_rate = config.sampleRate;     // Частота дискретизації
  cfg.channels = 2;                        // Стеро (лівий + правий канал)
  cfg.bits_per_sample = 16;                // 16 біт на один семпл
  cfg.buffer_size = config.bufferSize;     // Розмір буфера

  // Запуск I2S потоку
  if (!out.begin(cfg)) {
    Serial.println("Error audio: failed to start I2S");
    return false;
  }

  // Збереження функції зворотного виклику та ініціалізація Maximilian
  audioCallback = config.callback;
  maximilian.begin(cfg);
  initialized = true;
  return true;
}

// Запуск обробки наступного аудіобуфера
// Має викликатись регулярно в главному циклі (loop)
void audio_run() {
  // Перевірка: модуль повинен бути ініціалізований перед запуском
  if (!initialized) {
    Serial.println("Error audio: module not initialized");
    return;
  }

  // Генеруємо та передаємо наступний аудіобуфер
  maximilian.copy();
}
