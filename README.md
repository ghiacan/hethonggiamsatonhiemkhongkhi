🧩 Software Components

Arduino IDE (ESP32)

- Python (Flask) – Web Server

- SQLite – Data Storage

- MQTT (HiveMQ Cloud)

- HTML / CSS / JavaScript

- Chatbot

⚙️ Operating Principle
🔹 Sensor Node

- Reads PM2.5 and CO₂ data

- Displays directly on LCD

- Sends data via LoRa

🔹 ESP32 Gateway

- Receives data from multiple LoRa nodes

- Classifies by Node ID

- Sends data to MQTT Cloud

- Displays data for each node on LCD (using alternating button presses)

🔹 Server & Web

- Receives MQTT data

- Saves to SQLite

- Displays real-time on Web Dashboard

- Displays alerts when thresholds are exceeded

🚨 Alert Function

- Direct alerts on the web interface

- Display pop-ups (Messenger-style)

- Separate cooldown for each node

- Supports multiple nodes alerting simultaneously

📊 Main Features

✅ Real-time PM2.5 & CO₂ monitoring

✅ Long-range data transmission using LoRa

✅ MQTT Cloud

✅ Web Dashboard

✅ Threshold exceeding alerts

✅ AI chatbot for data querying

✅ Statistics by minute / hour / day

✅ Supports multiple nodes
