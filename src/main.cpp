#include <WiFi.h>
#include <PubSubClient.h>

// Налаштування Wi-Fi
const char* ssid = "PLAY_Swiatlowod_89BC";
const char* password = "zgxNMtN5f&n$";

// Налаштування MQTT брокера (публічний сервер HiveMQ)
const char* mqtt_server = "broker.hivemq.com";
const char* topic = "esp32s3/test";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

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
      client.subscribe("esp32s3/commands"); // Підписка на вхідні команди
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
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(100);  // Затримка для ініціалізації Serial на ESP32-S3
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    const char* msg = "Hello from ESP32-S3!";
    client.publish(topic, msg);
    Serial.print("Published: ");
    Serial.println(msg);
  }
}
