#include <Arduino.h>

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

String nodeID = "Node2";

// --- Hàm đọc PM2.5 GP2Y1010 ổn định ---
float readPM25_stable() {
  float sum = 0;
  const int n = 10;

  // Bật LED làm nóng trước khi đọc
  for(int i=0; i<3; i++){
    digitalWrite(LED_PWR, HIGH);
    delayMicroseconds(280);
    digitalWrite(LED_PWR, LOW);
    delay(50);
  }

  // Đọc trung bình nhiều lần
  for(int i=0; i<n; i++){
    digitalWrite(LED_PWR, HIGH);
    delayMicroseconds(280);
    int val = analogRead(PM25_PIN);
    digitalWrite(LED_PWR, LOW);

    float voltage = val * (3.3 / 4095.0);

    float pm25 = 0.172 * voltage - 0.1;
    if(pm25 < 0) pm25 = 0;

    sum += pm25;
    delay(10);
  }
  return sum / n;
}


// --- Hàm đọc CO2 MQ-135 trung bình nhiều lần ---
float readCO2() {
  const int n = 30;
  float sum = 0;

  for (int i = 0; i < n; i++) {
    int raw = analogRead(MQ135_PIN);       // ADC 0–4095
    sum += raw;
    delay(20);
  }

  float adc = sum / n;                      // trung bình ADC
  float voltage = adc * (3.3 / 4095.0);     // đổi sang Volt (0–3.3V)

  // ---- SCALE TƯƠNG ĐỐI ----
  // MQ135 điện áp thấp ~ sạch (400 ppm)
  // MQ135 điện áp cao ~ ô nhiễm (5000 ppm)
  float ppm = (voltage - 0.2) * (5000.0 / (2.8 - 0.2));  

  // ---- GIỚI HẠN ----
  if (ppm < 400) ppm = 400;
  if (ppm > 5000) ppm = 5000;

  return ppm;
}

void setup() {
  Serial.begin(115200);

  // --- Thiết lập M0/M1 ở Mode Normal ---
  pinMode(M0_PIN, OUTPUT);
  pinMode(M1_PIN, OUTPUT);
  digitalWrite(M0_PIN, LOW);
  digitalWrite(M1_PIN, LOW);

  pinMode(LED_PWR, OUTPUT);

  Serial.println("ESP32 Node PM2.5 + CO2 với LoRa sẵn sàng");

  // Delay ban đầu để ESP32 và cảm biến ổn định
  delay(5000);

  // Khởi tạo LoRa sau khi ESP32 ổn định
  Serial2.begin(9600, SERIAL_8N1, LORA_RX, LORA_TX);

}

void loop() {
  // --- Đọc PM2.5 và CO2 ---
  float pm25 = readPM25_stable();
  float co2 = readCO2();

  // --- Chuỗi dữ liệu CSV ---
  String data = nodeID + ",";
  data += "PM25:" + String(pm25, 1) + ",";
  data += "CO2:" + String(co2, 0);

  // --- Gửi qua LoRa ---
  Serial2.println(data);

  // --- In Serial monitor ---
  Serial.print("Đang gửi: ");
  Serial.println(data);

  // --- Delay giữa các lần gửi để tránh tải đột ngột ---
  delay(3000);
}
