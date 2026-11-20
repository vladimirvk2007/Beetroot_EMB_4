#include "audio.h"

#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"

namespace {
bool initialized = false;

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

bool beginOutput(const audio_config_t &config) {
  if (initialized) {
    Serial.println("Error [audio]: module is already initialized");
    return false;
  }
  if (config.sampleRate <= 0 || config.bufferSize <= 0 ||
      config.pinBck < 0 || config.pinWs < 0 ||
      config.pinData < 0 || config.pinBck == config.pinWs ||
      config.pinBck == config.pinData || config.pinWs == config.pinData) {
    Serial.println("Error [audio]: invalid configuration");
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
    Serial.println("Error [audio]: failed to start I2S");
    return false;
  }

  return true;
}
}

bool audio_mp3_init(const audio_config_t &config) {
  if (!beginOutput(config)) {
    return false;
  }
  if (!mp3Stream().begin() || !mp3Decoder()) {
    Serial.println("Error [audio]: failed to start MP3 decoder");
    mp3Stream().end();
    audioOutput().end();
    return false;
  }

  initialized = true;
  return true;
}

bool audio_mp3_reset() {
  if (!initialized) {
    Serial.println("Error [audio]: MP3 decoder is not initialized");
    return false;
  }
  mp3Stream().end();
  if (!mp3Stream().begin() || !mp3Decoder()) {
    Serial.println("Error [audio]: failed to restart MP3 decoder");
    return false;
  }
  return true;
}

size_t audio_mp3_write(const uint8_t *data, size_t size) {
  if (!initialized || !mp3Decoder() || data == nullptr) {
    Serial.println("Error [audio]: cannot send data to MP3 decoder");
    return 0;
  }
  return mp3Stream().write(data, size);
}
