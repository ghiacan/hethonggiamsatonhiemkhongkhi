#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/* ================== WIFI ================== */
const char* WIFI_SSID = "Nghia";
const char* WIFI_PASSWORD = "11223344";

/* ================== MQTT ================== */
const char* MQTT_SERVER = "140c1fe0108343549da9b96c9afed071.s1.eu.hivemq.cloud";
const uint16_t MQTT_PORT = 8883;
const char* MQTT_USER = "ducnghia";
const char* MQTT_PASS = "Asd3092003";
const char* MQTT_CLIENT_ID = "ESP32_Gateway";
const char* MQTT_TOPIC = "iot/air/";

/* ================== LoRa AS32 ================== */
HardwareSerial LoRaSerial(2);
#define LORA_RX 16
#define LORA_TX 17
#define M0_PIN  25
#define M1_PIN  26

/* ================== LCD + BUTTON ================== */
LiquidCrystal_I2C lcd(0x27, 16, 2);
#define BTN_PIN 27

/* ================== MQTT CLIENT ================== */
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

/* ================== DATA ================== */
float pm25_1 = 0, co2_1 = 0;
float pm25_2 = 0, co2_2 = 0;

/* ================== STATE ================== */
int currentScreen = 0;               // 0 = Node1 | 1 = Node2
unsigned long lastBtnTime = 0;
unsigned long lastLCDUpdate = 0;
const unsigned long LCD_INTERVAL = 1000;

/* ================== WIFI ================== */
void setupWiFi() {
  Serial.print("[WiFi] Connecting");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[WiFi] Connected");
}

/* ================== MQTT ================== */
void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("[MQTT] Connecting...");
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
      Serial.println("OK");
    } else {
      Serial.print(" FAIL rc=");
      Serial.println(mqttClient.state());
      delay(3000);
    }
  }
}
/* ================== PARSE DATA ================== */
void parseData(const String& payload) {
  int p1 = payload.indexOf("PM25:");
  int p2 = payload.indexOf(",CO2:");
  if (p1 < 0 || p2 < 0) return;

  float pm  = payload.substring(p1 + 5, p2).toFloat();
  float co2 = payload.substring(p2 + 5).toFloat();

  if (payload.startsWith("Node1")) {
    pm25_1 = pm;
    co2_1  = co2;
  } 
  else if (payload.startsWith("Node2")) {
    pm25_2 = pm;
    co2_2  = co2;
  }
  updateLCD();
}

/* ================== FAST LORA RX (CÁCH 2) ================== */
void readLoRaFast() {
  static String rx = "";
  static unsigned long lastByteTime = 0;

  while (LoRaSerial.available()) {
    char c = LoRaSerial.read();
    lastByteTime = millis();

    if (c == '\n') {
      rx.trim();
      Serial.print("[LoRa RX] ");
      Serial.println(rx);

      if (rx.startsWith("Node")) {
        parseData(rx);

        String topic = String(MQTT_TOPIC) +
                       (rx.startsWith("Node1") ? "node1" : "node2");
        mqttClient.publish(topic.c_str(), rx.c_str());
      }

      rx = "";
    } else {
      rx += c;
    }
  }

  // reset nếu frame lỗi
  if (rx.length() > 0 && millis() - lastByteTime > 50) {
    rx = "";
  }
}

/* ================== LCD ================== */
void updateLCD() {
  lcd.clear();
  if (currentScreen == 0) {
    lcd.setCursor(0, 0);
    lcd.print("Node 1");
    lcd.setCursor(0, 1);
    lcd.print("PM:");
    lcd.print(pm25_1, 1);
    lcd.print(" CO2:");
    lcd.print(co2_1, 1);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Node 2");
    lcd.setCursor(0, 1);
    lcd.print("PM:");
    lcd.print(pm25_2, 1);
    lcd.print(" CO2:");
    lcd.print(co2_2, 1);
  }
}

/* ================== SETUP ================== */
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
  lcd.print("Gateway Start");

  setupWiFi();

  espClient.setInsecure();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  delay(1000);
  lcd.clear();
}

/* ================== LOOP ================== */
void loop() {
  readLoRaFast();        

  if (!mqttClient.connected()) connectMQTT();
  mqttClient.loop();

  // ---- Button toggle Node1 <-> Node2 ----
  if (digitalRead(BTN_PIN) == LOW) {
    if (millis() - lastBtnTime > 300) {
      currentScreen = !currentScreen;
      lastBtnTime = millis();
      updateLCD();              // đổi màn hình NGAY
      Serial.print("[BTN] Screen = ");
      Serial.println(currentScreen);
    }
  }

  // ---- periodic LCD refresh ----
  if (millis() - lastLCDUpdate > LCD_INTERVAL) {
    updateLCD();
    lastLCDUpdate = millis();
  }
}
