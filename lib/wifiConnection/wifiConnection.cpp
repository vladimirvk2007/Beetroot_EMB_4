#include "wifiConnection.h"

#include <Arduino.h>
#include <WiFi.h>

namespace {
const uint32_t WIFI_RETRY_INTERVAL_MS = 15000;

const char *networkSsid = nullptr;
const char *networkPassword = nullptr;
bool initialized = false;
bool wasConnected = false;
uint32_t lastRetry = 0;
}

bool wifiConnection_init(const char *ssid, const char *password) {
  if (initialized) {
    Serial.println("Error [Wi-Fi]: module is already initialized");
    return false;
  }
  if (ssid == nullptr || ssid[0] == '\0' || password == nullptr) {
    Serial.println("Error [Wi-Fi]: invalid network configuration");
    return false;
  }

  WiFi.persistent(false);
  if (!WiFi.mode(WIFI_STA)) {
    Serial.println("Error: failed to start Wi-Fi");
    return false;
  }
  networkSsid = ssid;
  networkPassword = password;
  WiFi.setAutoReconnect(true);
  WiFi.begin(networkSsid, networkPassword);
  Serial.println("Connecting to Wi-Fi...");
  lastRetry = millis();
  initialized = true;
  return true;
}

bool wifiConnection_run() {
  if (!initialized) {
    Serial.println("Error [Wi-Fi]: module is not initialized");
    return false;
  }

  bool connected = WiFi.status() == WL_CONNECTED;
  if (connected != wasConnected) {
    Serial.println(connected ? "Wi-Fi connected" : "Wi-Fi disconnected");
    wasConnected = connected;
  }
  uint32_t now = millis();
  if (!connected && now - lastRetry >= WIFI_RETRY_INTERVAL_MS) {
    Serial.println("Reconnecting to Wi-Fi...");
    WiFi.begin(networkSsid, networkPassword);
    lastRetry = now;
  }
  return connected;
}
