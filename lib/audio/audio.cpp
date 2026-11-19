#include "audio.h"

#include "AudioTools.h"
#include "AudioTools/AudioLibs/MaximilianDSP.h"

namespace {
I2SStream out;
audio_callback_t audioCallback = nullptr;
bool initialized = false;

void renderAudio(float *output) {
  audioCallback(output);
}

Maximilian maximilian(out, DEFAULT_BUFFER_SIZE, renderAudio);
}

bool audio_init(const audio_config_t &config) {
  if (initialized) {
    Serial.println("Помилка audio: модуль уже ініціалізовано");
    return false;
  }
  if (config.callback == nullptr || config.sampleRate <= 0 ||
      config.bufferSize <= 0 || config.pinBck < 0 || config.pinWs < 0 ||
      config.pinData < 0 || config.pinBck == config.pinWs ||
      config.pinBck == config.pinData || config.pinWs == config.pinData) {
    Serial.println("Помилка audio: некоректна конфігурація");
    return false;
  }

  auto cfg = out.defaultConfig(TX_MODE);
  cfg.is_master = true;
  cfg.pin_bck = config.pinBck;
  cfg.pin_ws = config.pinWs;
  cfg.pin_data = config.pinData;
  cfg.sample_rate = config.sampleRate;
  cfg.channels = 2;
  cfg.bits_per_sample = 16;
  cfg.buffer_size = config.bufferSize;

  if (!out.begin(cfg)) {
    Serial.println("Помилка audio: не вдалося запустити I2S");
    return false;
  }

  audioCallback = config.callback;
  maximilian.begin(cfg);
  initialized = true;
  return true;
}

void audio_run() {
  if (!initialized) {
    Serial.println("Помилка audio: модуль не ініціалізовано");
    return;
  }

  maximilian.copy();
}
