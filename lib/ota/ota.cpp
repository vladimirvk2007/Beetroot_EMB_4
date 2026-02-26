#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>

#include "ota.h"
#include "credentials.h"

// --- Wi-Fi credentials ---
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// --- OTA Callbacks ---
static void otaStart() {
    Serial.println("Start updating...");
}

static void otaEnd() {
    Serial.println("Update finished!");
}

static void otaProgress(unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
}

static void otaError(ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
}

void otaSetup(void) {
// --- Connect to Wi-Fi ---
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // --- OTA Setup ---
    ArduinoOTA.setHostname("esp32-ota");
    ArduinoOTA.onStart(otaStart);
    ArduinoOTA.onEnd(otaEnd);
    ArduinoOTA.onProgress(otaProgress);
    ArduinoOTA.onError(otaError);
    ArduinoOTA.begin();
}

void otaHandle(void) {
    ArduinoOTA.handle();
}
