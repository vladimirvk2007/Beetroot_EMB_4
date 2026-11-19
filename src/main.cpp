#include <Arduino.h>
#include "audio.h"

#define I2S_BCK_PIN 15
#define I2S_WS_PIN 17
#define I2S_DATA_PIN 16

#define MODE_RADIO 1
#define MODE_TONE_TEST 2
#ifndef AUDIO_MODE
#define AUDIO_MODE MODE_RADIO
#endif

#if AUDIO_MODE == MODE_TONE_TEST
#include "maximilian.h"

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
  Serial.println("Запуск генератора тонів...");

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
#elif AUDIO_MODE == MODE_RADIO
#include <WiFi.h>
#include "AudioTools.h"
#include "AudioTools/Communication/AudioHttp.h"

#if __has_include("radio_config.h")
#include "radio_config.h"
#else
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#endif

const char *RADIO_URL = "http://icepe6.infomaniak.ch:80/jazz-wr01-128.mp3";
const uint32_t RETRY_INTERVAL_MS = 5000;
const uint32_t WIFI_RETRY_INTERVAL_MS = 15000;
const uint32_t STREAM_TIMEOUT_MS = 10000;

static bool radioReady = false;
static bool streamConnected = false;
static uint32_t lastRetry = 0;
static uint32_t lastData = 0;
static uint8_t inputBuffer[1024];

URLStream &radioStream() {
  static URLStream stream;
  return stream;
}

void setup() {
  Serial.begin(115200);
  if (WIFI_SSID[0] == '\0') {
    Serial.println("Помилка: налаштуйте Wi-Fi у include/radio_config.h");
    return;
  }

  const audio_config_t config = {
    I2S_BCK_PIN,
    I2S_WS_PIN,
    I2S_DATA_PIN,
    44100,
    256,
    nullptr
  };
  if (!audio_mp3_init(config)) {
    return;
  }

  WiFi.persistent(false);
  if (!WiFi.mode(WIFI_STA)) {
    Serial.println("Помилка запуску Wi-Fi");
    return;
  }
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Підключення до Wi-Fi...");
  lastRetry = millis();
  radioReady = true;
}

void loop() {
  if (!radioReady) {
    delay(1000);
    return;
  }

  URLStream &stream = radioStream();
  uint32_t now = millis();
  if (WiFi.status() != WL_CONNECTED) {
    if (streamConnected) {
      Serial.println("Wi-Fi відключено");
      stream.end();
      streamConnected = false;
    }
    if (now - lastRetry >= WIFI_RETRY_INTERVAL_MS) {
      Serial.println("Повторне підключення до Wi-Fi...");
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      lastRetry = now;
    }
    delay(10);
    return;
  }

  if (!streamConnected) {
    if (now - lastRetry < RETRY_INTERVAL_MS) {
      delay(10);
      return;
    }
    lastRetry = now;
    if (!audio_mp3_reset()) {
      return;
    }
    Serial.println("Підключення до радіостанції...");
    if (!stream.begin(RADIO_URL, "audio/mpeg")) {
      Serial.println("Помилка підключення до радіостанції");
      stream.end();
      lastRetry = millis();
      return;
    }
    streamConnected = true;
    lastData = millis();
    Serial.println("Радіопотік підключено");
  }

  int available = stream.available();
  size_t received = 0;
  if (available > 0) {
    size_t toRead = min(static_cast<size_t>(available), sizeof(inputBuffer));
    received = stream.readBytes(inputBuffer, toRead);
    if (received > 0) {
      lastData = millis();
      if (audio_mp3_write(inputBuffer, received) != received) {
        Serial.println("Помилка декодування MP3; перепідключення...");
        stream.end();
        streamConnected = false;
        lastRetry = millis();
      }
    }
  }
  if (received == 0) {
    if (streamConnected && (!stream.httpRequest().connected() ||
        millis() - lastData >= STREAM_TIMEOUT_MS)) {
      Serial.println("Радіопотік перервано; перепідключення...");
      stream.end();
      streamConnected = false;
      lastRetry = millis();
    }
    delay(10);
  }
}
#else
#error "Невідомий режим аудіо"
#endif
