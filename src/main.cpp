#include <WiFi.h>
#include <PubSubClient.h>
#include "credentials.h"
#include "broker.h"

#define UART_BAUD_RATE 115200

// Налаштування Wi-Fi
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// Налаштування MQTT брокера (публічний сервер HiveMQ)
const char* mqtt_server = MQTT_SERVER;
const int mqtt_port = MQTT_PORT;
const char* topic = MQTT_TOPIC;
const char* commands_topic = MQTT_COMMANDS;
const char* status_topic = MQTT_STATUS;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
unsigned long msgCount = 0;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Створюємо унікальний ID клієнта
    String clientId = "ESP32S3Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(commands_topic); // Підписка на вхідні команди
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Функція обробки вхідних повідомлень
void callback(char* topic, byte* payload, unsigned int length) {
  // Копіюємо payload у рядок для зручного порівняння
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(message);

  // Обробка команд з топіку esp32s3/commands
  if (String(topic) == commands_topic) {
    if (message == "ON") {
      Serial.println("Command: LED ON");
      // digitalWrite(LED_PIN, HIGH);  // розкоментуйте якщо є LED
    } else if (message == "OFF") {
      Serial.println("Command: LED OFF");
      // digitalWrite(LED_PIN, LOW);   // розкоментуйте якщо є LED
    } else if (message == "STATUS") {
      client.publish(status_topic, "ESP32-S3 is running");
      Serial.println("Status sent");
    } else {
      Serial.println("Unknown command");
    }
  }
}

void setup() {
  Serial.begin(UART_BAUD_RATE);
  delay(100);  // Затримка для ініціалізації Serial на ESP32-S3
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;
    msgCount++;
    char msg[64];
    snprintf(msg, sizeof(msg), "Hello from ESP32-S3! #%lu", msgCount);
    client.publish(topic, msg);
    Serial.print("Published: ");
    Serial.println(msg);
  }
}
