#include "audio.h"

#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "AudioTools/AudioLibs/MaximilianDSP.h"

namespace {
audio_callback_t audioCallback = nullptr;
bool initialized = false;
bool mp3Mode = false;

I2SStream &audioOutput() {
  static I2SStream out;
  return out;
}

MP3DecoderHelix &mp3Decoder() {
  static MP3DecoderHelix mp3;
  return mp3;
}

EncodedAudioStream &mp3Stream() {
  static EncodedAudioStream decoder(&audioOutput(), &mp3Decoder());
  return decoder;
}

void renderAudio(float *output) {
  audioCallback(output);
}

Maximilian &toneGenerator() {
  static Maximilian maximilian(audioOutput(), DEFAULT_BUFFER_SIZE, renderAudio);
  return maximilian;
}

bool beginOutput(const audio_config_t &config) {
  if (initialized) {
    Serial.println("Помилка audio: модуль уже ініціалізовано");
    return false;
  }
  if (config.sampleRate <= 0 || config.bufferSize <= 0 ||
      config.pinBck < 0 || config.pinWs < 0 ||
      config.pinData < 0 || config.pinBck == config.pinWs ||
      config.pinBck == config.pinData || config.pinWs == config.pinData) {
    Serial.println("Помилка audio: некоректна конфігурація");
    return false;
  }

  I2SStream &out = audioOutput();
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

  return true;
}
}

bool audio_init(const audio_config_t &config) {
  if (config.callback == nullptr) {
    Serial.println("Помилка audio: не задано функцію генерації");
    return false;
  }
  if (!beginOutput(config)) {
    return false;
  }

  audioCallback = config.callback;
  toneGenerator().begin(audioOutput().audioInfo());
  initialized = true;
  return true;
}

void audio_run() {
  if (!initialized || mp3Mode) {
    Serial.println("Помилка audio: генератор не ініціалізовано");
    return;
  }

  toneGenerator().copy();
}

bool audio_mp3_init(const audio_config_t &config) {
  if (!beginOutput(config)) {
    return false;
  }
  if (!mp3Stream().begin() || !mp3Decoder()) {
    Serial.println("Помилка audio: не вдалося запустити декодер MP3");
    mp3Stream().end();
    audioOutput().end();
    return false;
  }

  mp3Mode = true;
  initialized = true;
  return true;
}

bool audio_mp3_reset() {
  if (!initialized || !mp3Mode) {
    Serial.println("Помилка audio: декодер MP3 не ініціалізовано");
    return false;
  }
  mp3Stream().end();
  if (!mp3Stream().begin() || !mp3Decoder()) {
    Serial.println("Помилка audio: не вдалося перезапустити декодер MP3");
    return false;
  }
  return true;
}

size_t audio_mp3_write(const uint8_t *data, size_t size) {
  if (!initialized || !mp3Mode || !mp3Decoder() || data == nullptr) {
    Serial.println("Помилка audio: неможливо передати дані до декодера MP3");
    return 0;
  }
  return mp3Stream().write(data, size);
}
