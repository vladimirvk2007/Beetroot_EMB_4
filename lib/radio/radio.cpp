#include "radio.h"

#include <Arduino.h>
#include <WiFi.h>
#include "AudioTools.h"
#include "AudioTools/Communication/AudioHttp.h"

namespace {
const uint32_t RETRY_INTERVAL_MS = 5000;
const uint32_t WIFI_RETRY_INTERVAL_MS = 15000;
const uint32_t STREAM_TIMEOUT_MS = 10000;

radio_config_t radioConfig;
bool radioReady = false;
bool streamConnected = false;
uint32_t lastRetry = 0;
uint32_t lastData = 0;
uint8_t inputBuffer[1024];

URLStream &radioStream() {
  static URLStream stream;
  return stream;
}
}

bool radio_init(const radio_config_t &config) {
  if (radioReady) {
    Serial.println("Error [radio]: module is already initialized");
    return false;
  }
  if (config.ssid == nullptr || config.ssid[0] == '\0' ||
      config.password == nullptr || config.url == nullptr || config.url[0] == '\0') {
    Serial.println("Error [radio]: invalid Wi-Fi or stream configuration");
    return false;
  }
  if (!audio_mp3_init(config.audio)) {
    return false;
  }

  WiFi.persistent(false);
  if (!WiFi.mode(WIFI_STA)) {
    Serial.println("Error: failed to start Wi-Fi");
    return false;
  }
  radioConfig = config;
  WiFi.setAutoReconnect(true);
  WiFi.begin(radioConfig.ssid, radioConfig.password);
  Serial.println("Connecting to Wi-Fi...");
  lastRetry = millis();
  radioReady = true;
  return true;
}

void radio_run() {
  if (!radioReady) {
    delay(1000);
    return;
  }

  URLStream &stream = radioStream();
  uint32_t now = millis();
  if (WiFi.status() != WL_CONNECTED) {
    if (streamConnected) {
      Serial.println("Wi-Fi disconnected");
      stream.end();
      streamConnected = false;
    }
    if (now - lastRetry >= WIFI_RETRY_INTERVAL_MS) {
      Serial.println("Reconnecting to Wi-Fi...");
      WiFi.begin(radioConfig.ssid, radioConfig.password);
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
    Serial.println("Connecting to radio station...");
    if (!stream.begin(radioConfig.url, "audio/mpeg")) {
      Serial.println("Error: failed to connect to radio station");
      stream.end();
      lastRetry = millis();
      return;
    }
    streamConnected = true;
    lastData = millis();
    Serial.println("Radio stream connected");
  }

  int available = stream.available();
  size_t received = 0;
  if (available > 0) {
    size_t toRead = min(static_cast<size_t>(available), sizeof(inputBuffer));
    received = stream.readBytes(inputBuffer, toRead);
    if (received > 0) {
      lastData = millis();
      if (audio_mp3_write(inputBuffer, received) != received) {
        Serial.println("MP3 decoding error; reconnecting...");
        stream.end();
        streamConnected = false;
        lastRetry = millis();
      }
    }
  }
  if (received == 0) {
    if (streamConnected && (!stream.httpRequest().connected() ||
        millis() - lastData >= STREAM_TIMEOUT_MS)) {
      Serial.println("Radio stream interrupted; reconnecting...");
      stream.end();
      streamConnected = false;
      lastRetry = millis();
    }
    delay(10);
  }
}
