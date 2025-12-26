#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================== WiFi ==================
const char* WIFI_SSID     = "Nghia";
const char* WIFI_PASSWORD = "11223344";

// ================== MQTT Cloud ==================
const char* MQTT_SERVER   = "140c1fe0108343549da9b96c9afed071.s1.eu.hivemq.cloud";
const uint16_t MQTT_PORT  = 8883;
const char* MQTT_USER     = "ducnghia";
const char* MQTT_PASS     = "Asd3092003";
const char* MQTT_CLIENT_ID = "Gateway";

const char* MQTT_TOPIC = "iot/air/";

// ================== LoRa ==================
HardwareSerial LoRaSerial(2);
#define LORA_RX 16
#define LORA_TX 17
#define M0_PIN  25
#define M1_PIN  26

// ================== LCD + Button ==================
LiquidCrystal_I2C lcd(0x27, 16, 2);
#define BTN_PIN 27
int currentScreen = 0;
unsigned long lastBtnTime = 0;

// ================== BIẾN ==================
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

// dữ liệu Node
float pm25_1 = 0, co2_1 = 0;
float pm25_2 = 0, co2_2 = 0;


// ================== WIFI ==================
void setupWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

// ================== MQTT ==================
void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting MQTT...");
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
      Serial.println("OK");
    } else {
      delay(3000);
    }
  }
}

// ================== PARSE DATA ==================
void parseData(String payload) {
  // VD: Node1,PM25:35.2,CO2:820
  if (payload.indexOf("Node1") >= 0) {
    pm25_1 = payload.substring(payload.indexOf("PM25:") + 5, payload.indexOf(",CO2")).toFloat();
    co2_1  = payload.substring(payload.indexOf("CO2:") + 4).toFloat();
  }
  else if (payload.indexOf("Node2") >= 0) {
    pm25_2 = payload.substring(payload.indexOf("PM25:") + 5, payload.indexOf(",CO2")).toFloat();
    co2_2  = payload.substring(payload.indexOf("CO2:") + 4).toFloat();
  }
}

// ================== LCD ==================
void updateLCD() {
  lcd.clear();
  if (currentScreen == 0) {
    lcd.setCursor(0, 0);
    lcd.print("Node 1");
    lcd.setCursor(0, 1);
    lcd.print("PM:");
    lcd.print(pm25_1, 1);
    lcd.print(" CO2:");
    lcd.print((int)co2_1);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Node 2");
    lcd.setCursor(0, 1);
    lcd.print("PM:");
    lcd.print(pm25_2, 1);
    lcd.print(" CO2:");
    lcd.print((int)co2_2);
  }
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  // LoRa
  LoRaSerial.begin(9600, SERIAL_8N1, LORA_RX, LORA_TX);
  pinMode(M0_PIN, OUTPUT);
  pinMode(M1_PIN, OUTPUT);
  digitalWrite(M0_PIN, LOW);
  digitalWrite(M1_PIN, LOW);

  // Button
  pinMode(BTN_PIN, INPUT_PULLUP);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.print("Gateway Starting");

  setupWiFi();
  espClient.setInsecure();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  delay(1000);
  lcd.clear();
}

// ================== LOOP ==================
void loop() {
  if (!mqttClient.connected()) connectMQTT();
  mqttClient.loop();

  // Đọc LoRa
  if (LoRaSerial.available()) {
    String data = LoRaSerial.readStringUntil('\n');
    data.trim();
    Serial.println(data);

    parseData(data);

    String topic = String(MQTT_TOPIC) + (data.startsWith("Node1") ? "node1" : "node2");
    mqttClient.publish(topic.c_str(), data.c_str());

    updateLCD();
  }

  // Đọc nút nhấn (debounce)
  if (digitalRead(BTN_PIN) == LOW) {      
  if (millis() - lastBtnTime > 300) {     
    currentScreen = !currentScreen;    
    updateLCD();                          
    lastBtnTime = millis();
  }
}
}
