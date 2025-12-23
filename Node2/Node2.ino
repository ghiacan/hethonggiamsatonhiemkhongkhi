#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HardwareSerial.h>

// ================== WiFi ==================
const char* WIFI_SSID     = "Nghia";
const char* WIFI_PASSWORD = "11223344";

// ================== MQTT Cloud (HiveMQ) ==================
const char* MQTT_SERVER   = "140c1fe0108343549da9b96c9afed071.s1.eu.hivemq.cloud";
const uint16_t MQTT_PORT  = 8883;
const char* MQTT_USER     = "ducnghia";
const char* MQTT_PASS     = "Asd3092003";

// Client ID: phải duy nhất
const char* MQTT_CLIENT_ID = "Gateway";

const char* MQTT_TOPIC = "iot/air/";

// ================== LoRa AS32 (UART2) ==================
HardwareSerial LoRaSerial(2);
#define LORA_RX 16
#define LORA_TX 17
#define M0_PIN  25
#define M1_PIN  26

// ================== BIẾN TOÀN CỤC ==================
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

unsigned long lastMqttReconnectAttempt = 0;
const unsigned long MQTT_RECONNECT_INTERVAL = 5000;

// ================== HÀM PHỤ TRỢ ==================
void setupWiFi() {
  Serial.print("Kết nối WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.print("IP ESP32 Gateway: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n Không kết nối được WiFi (timeout)");
  }
}

bool connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(" WiFi chưa kết nối, bỏ qua MQTT");
    return false;
  }

  Serial.print("Đang kết nối MQTT Cloud tới ");
  Serial.print(MQTT_SERVER);
  Serial.print(":");
  Serial.print(MQTT_PORT);
  Serial.print(" ... ");

  bool ok = mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS);

  if (ok) {
    Serial.println("OK");
  } else {
    Serial.print("Failed, rc=");
    Serial.println(mqttClient.state());
  }
  return ok;
}

void ensureMQTTConnected() {
  if (!mqttClient.connected()) {
    unsigned long now = millis();
    if (now - lastMqttReconnectAttempt > MQTT_RECONNECT_INTERVAL) {
      lastMqttReconnectAttempt = now;
      connectMQTT();
    }
  }
  mqttClient.loop();
}

// Lấy node từ payload: "Node1,PM25:...,CO2:..."
String extractNodeId(const String& payload) {
  int comma = payload.indexOf(',');
  if (comma <= 0) return "";
  String node = payload.substring(0, comma);
  node.trim();
  return node; // vd: "Node1"
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);
  delay(200);

  // LoRa UART2
  LoRaSerial.begin(9600, SERIAL_8N1, LORA_RX, LORA_TX);

  // M0/M1 normal mode
  pinMode(M0_PIN, OUTPUT);
  pinMode(M1_PIN, OUTPUT);
  digitalWrite(M0_PIN, LOW);
  digitalWrite(M1_PIN, LOW);

  setupWiFi();

  // MQTT secure: demo dùng insecure
  espClient.setInsecure();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  Serial.println("ESP32 Gateway MQTT Cloud ready");
}

// ================== LOOP ==================
void loop() {
  ensureMQTTConnected();

  if (LoRaSerial.available()) {
    String data = LoRaSerial.readStringUntil('\n');
    data.trim();
    if (data.length() == 0) return;
    Serial.println(data);

    // Tạo topic theo node
    String nodeId = extractNodeId(data);
    if (nodeId.length() == 0) {
      Serial.println(" Không lấy được nodeId từ payload!");
      return;
    }

    String topic = String(MQTT_TOPIC) + nodeId;   
    topic.toLowerCase(); 

    if (mqttClient.connected()) {
      bool ok = mqttClient.publish(topic.c_str(), data.c_str());
      if (ok) {
        Serial.print("Đã gửi Data lên MQTT cloud: ");
        Serial.println(data);
      } else {
        Serial.println(" Gửi MQTT thất bại (publish return false)");
      }
    } else {
      Serial.println(" MQTT chưa kết nối, không gửi được");
    }
  }

  delay(5);
}