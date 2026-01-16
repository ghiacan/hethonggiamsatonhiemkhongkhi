#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/* ================== LoRa AS32 TTL ================== */
#define LORA_TX 17
#define LORA_RX 16
#define M0_PIN  25
#define M1_PIN  26

/* ================== GP2Y1010 PM2.5 ================= */
#define LED_PWR   4
#define PM25_PIN  34

/* ================== MQ-135 CO2 ===================== */
#define MQ135_PIN 35

/* ================== LCD ============================ */
LiquidCrystal_I2C lcd(0x27, 16, 2);

/* ================== NODE INFO ====================== */
String nodeID = "Node2";

/* ================== TIMING ========================= */
unsigned long lastSend = 0;
const unsigned long SEND_INTERVAL = 12000;   

/* ================== SENSOR ========================= */
float pm25 = 0, co2 = 0;

/* ================== DATA ĐÃ GỬI ==================== */
float pm25_sent = 0, co2_sent = 0;

String data = "";

/* ================== PM2.5 ========================== */
float readPM25_stable() {
  float sum = 0;
  const int n = 10;

  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PWR, HIGH);
    delayMicroseconds(280);
    digitalWrite(LED_PWR, LOW);
    delay(50);
  }

  for (int i = 0; i < n; i++) {
    digitalWrite(LED_PWR, HIGH);
    delayMicroseconds(280);
    int val = analogRead(PM25_PIN);
    digitalWrite(LED_PWR, LOW);

    float voltage = val * (3.3 / 4094.0);
    float pm = (voltage - 0.9) / 0.5;
    if (pm < 0) pm = 0;

    sum += pm;
    delay(10);
  }

  return sum / n;
}

/* ================== CO2 ============================ */
float readCO2() {
  const int n = 30;
  float sum = 0;

  for (int i = 0; i < n; i++) {
    sum += analogRead(MQ135_PIN);
    delay(20);
  }

  float adc = sum / n;
  float voltage = adc * (3.3 / 4095.0);
  float ppm = (voltage - 0.2) * (5000.0 / (2.8 - 0.2));
  return ppm;
}

/* ================== LCD UPDATE ===================== */
void updateLCD() {
  lcd.setCursor(0, 0);
  lcd.print("PM2.5:");
  lcd.print(pm25_sent, 1);
  lcd.print("   ");

  lcd.setCursor(0, 1);
  lcd.print("CO2:");
  lcd.print(co2_sent, 1);
  lcd.print(" ppm   ");
}

/* ================== SETUP ========================== */
void setup() {
  Serial.begin(115200);

  pinMode(M0_PIN, OUTPUT);
  pinMode(M1_PIN, OUTPUT);
  digitalWrite(M0_PIN, LOW);
  digitalWrite(M1_PIN, LOW);

  lastSend = millis() + 2500;  

  pinMode(LED_PWR, OUTPUT);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.print("Node2 Starting");

  Serial2.begin(9600, SERIAL_8N1, LORA_RX, LORA_TX);

  delay(2000);
  lcd.clear();
  lcd.print("Node2 Ready");

  Serial.println("Node2 ready");
}

/* ================== LOOP =========================== */
void loop() {

  if (millis() - lastSend >= SEND_INTERVAL) {
    lastSend = millis() ;

    // ĐO
    pm25 = readPM25_stable();
    co2  = readCO2();

    // BUILD PAYLOAD
    data = nodeID +
           ",PM25:" + String(pm25, 1) +
           ",CO2:"  + String(co2, 1);

    // SEND LORA
    for (int i = 0; i < data.length(); i++) {
      Serial2.write(data[i]);
      delay(2);
    }
    Serial2.write('\n');

    Serial.print("[TX] ");
    Serial.println(data);

    // ===== CHỈ SAU KHI GỬI =====
    pm25_sent = pm25;
    co2_sent  = co2;

    updateLCD();
  }
}
