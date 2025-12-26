#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================= LCD 16x2 =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- LoRa AS32 TTL ---
#define LORA_TX 17
#define LORA_RX 16
#define M0_PIN 25
#define M1_PIN 26

// --- GP2Y1010 PM2.5 ---
#define LED_PWR  4
#define PM25_PIN 34

// --- MQ-135 CO2 ---
#define MQ135_PIN 35

String nodeID = "Node1";

// ================= PM2.5 =================
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

    float voltage = val * (3.3 / 4095.0);
    float pm25 = 0.172 * voltage - 0.1;
    if (pm25 < 0) pm25 = 0;

    sum += pm25;
    delay(10);
  }
  return sum / n;
}

// ================= CO2 (MQ135 - gia tri tuong doi) =================
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
  if (ppm < 400) ppm = 400;
  if (ppm > 5000) ppm = 5000;

  return ppm;
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(M0_PIN, OUTPUT);
  pinMode(M1_PIN, OUTPUT);
  digitalWrite(M0_PIN, LOW);
  digitalWrite(M1_PIN, LOW);

  pinMode(LED_PWR, OUTPUT);

  // LCD
  Wire.begin(21, 22);   // SDA, SCL
  lcd.init();
  lcd.backlight();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Node1 Starting");
  lcd.setCursor(0, 1);
  lcd.print("Sensor Init...");
  
  delay(3000);

  Serial2.begin(9600, SERIAL_8N1, LORA_RX, LORA_TX);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Node1 Ready");
  lcd.setCursor(0, 1);
  lcd.print("LoRa OK");
}

// ================= LOOP =================
void loop() {
  float pm25 = readPM25_stable();
  float co2  = readCO2();

  // Chuoi gui LoRa
  String data = nodeID + ",";
  data += "PM25:" + String(pm25, 1) + ",";
  data += "CO2:" + String(co2, 0);

  Serial2.println(data);

  Serial.print("Dang gui: ");
  Serial.println(data);

  // ===== Hien thi LCD =====
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PM2.5:");
  lcd.print(pm25, 1);
  lcd.print(" ug");

  lcd.setCursor(0, 1);
  lcd.print("CO2:");
  lcd.print((int)co2);
  lcd.print(" ppm");

  delay(3000);
}