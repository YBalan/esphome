# JBD-BMS to MQTT Bridge with ESP32 & OLED Display

This project provides a comprehensive monitoring solution for **JBD / XiaoXiang / Jiabaida BMS** units using an **ESP32**. It connects via Bluetooth Low Energy (BLE) and pushes telemetry to a local **Mosquitto MQTT** broker. The system features an integrated OLED display with power-saving logic and automated relay control based on battery capacity.

## 🚀 Features
*   **JBD/XiaoXiang Integration:** Native support for JBD protocol via the `esphome-jbd-bms` driver.
*   **Local MQTT:** Real-time data transmission to a local server with password authentication.
*   **Automated Relay Logic:** Automatically toggles a system relay on **GPIO14** based on Capacity:
    *   **ON** when capacity drops below **160 Ah**.
    *   **OFF** when capacity reaches **195 Ah**.
*   **Smart OLED Display:** Integrated 0.96" screen showing Voltage, Current, and Capacity.
    *   **Auto-off:** Screen turns off after 30 seconds to prevent burn-in.
    *   **Manual Wake:** Use the physical **BOOT button** (GPIO0) on the ESP32 to wake the screen.
*   **Advanced Metrics:** 
    *   Calculates **Charging Time Remaining** (human-readable format).
    *   Monitors **Cell Imbalance (Delta)**.
    *   Tracks **Charging Cycles**.
*   **Connection Diagnostics:** Real-time monitoring of Wi-Fi Signal (RSSI) and BLE Link Strength.

---

## 🛠 Hardware Requirements
*   **Microcontroller:** ESP32 Ideaspark (or any ESP-WROOM-32 dev kit).
*   **BMS:** JBD / XiaoXiang / Jiabaida BLE-enabled BMS.
*   **Display:** Integrated 0.96" OLED (SSD1306) on I2C (GPIO 21/22).
*   **Relay:** 5V Opto-isolated relay module connected to **GPIO14**.
*   **Status Indicator:** Built-in Blue LED (GPIO2) synced with the relay state.

---

## 🔨 Installation & Compilation

### 1. Software Environment
*   **Python:** Install [Python 3.12.x](https://www.python.org/downloads/windows/). 
    *   *Note: Avoid Python 3.13 due to library compatibility issues.*
    *   *Ensure "Add Python to PATH" is checked during installation.*
*   **ESPHome:** Install via Command Prompt (CMD):
    ```cmd
    pip install esphome
    ```

### 2. Project Setup
1. Create a folder (e.g., `C:\jbdbms_monitor`).
2. Place `jbdbms_v2.yaml` and `secrets.yaml` in this folder.
3. **secrets.yaml** should contain:
   ```yaml
   wifi_ssid: "Your_WiFi_Name"
   wifi_password: "Your_WiFi_Password"
   mqtt_broker: "192.168.1.50" # Your PC IP
   mqtt_username: "yura"
   mqtt_password: "Your_MQTT_Password"
   ```

### 3. Flashing
Connect your ESP32 via USB and run:
```cmd
esphome run jbdbms_v2.yaml
```
*After the first USB flash, subsequent updates can be done **Over-The-Air (OTA)**.*

---

## 🖥 Mosquitto MQTT Broker Setup (Windows)

### 1. Configuration
In your `mosquitto.conf` (typically in `C:\Program Files\mosquitto`), use the following settings for local access:
```text
listener 1883
allow_anonymous false
password_file C:/mosquitto/passwords.txt
```

### 2. Security & Firewall
*   **Password File:** Create users using `mosquitto_passwd`. Ensure the file has `SYSTEM` permissions.
*   **Firewall:** Add an Inbound Rule for **TCP Port 1883**.

---

## 📊 MQTT Topic Map

### Telemetry (Subscribed via `jbdbms/`)
| Topic | Description |
|---|---|
| `jbdbms/voltage` | Total Battery Voltage (V) |
| `jbdbms/current` | Current (+ for Charge, - for Discharge) (A) |
| `jbdbms/soc` | Remaining Capacity (Ah) |
| `jbdbms/power` | Calculated Load/Charge Power (W) |
| `jbdbms/cycles` | Total Charging Cycles |
| `jbdbms/t1` / `jbdbms/t2` | Temperature sensors NTC1 / NTC2 (°C) |
| `jbdbms/min_c_v` / `jbdbms/max_c_v` | Cell Voltage extremes (V) |
| `jbdbms/delta` | Cell Imbalance (V) |
| `jbdbms/charging_time_human` | Time until full (e.g., "1h 45m") |
| `jbdbms/wifi_rssi` / `jbdbms/ble_rssi` | Signal Diagnostics (dBm) |

### Controls & Switches
| State Topic | Command Topic | Description |
|---|---|---|
| `jbdbms/relay/state` | `jbdbms/relay/set` | Manually toggle the GPIO14 Relay |
| `jbdbms/switch/charge/state` | `jbdbms/switch/charge/set` | Toggle BMS Charging MOSFET |
| `jbdbms/switch/discharge/state` | `jbdbms/switch/discharge/set` | Toggle BMS Discharging MOSFET |

---

## 🛠 Troubleshooting
*   **OLED Communication Failed:** Check I2C pins. If `21/22` fails, try `5/4` depending on your board revision.
*   **BLE Disconnects (rsn 0x08):** Ensure the official JBD app is closed on your phone. JBD BMS supports only one connection at a time.
*   **MQTT Connection Refused:** Verify that the `client_id` and `username/password` in your YAML match the Mosquitto broker settings exactly.
*   **OTA Refused:** Ensure the `ota:` block is present in the configuration and your computer can "ping" the ESP32 IP address.

---
*Disclaimer: This project involves high-current LiFePO4 batteries. Use proper fuses and safety equipment. Use at your own risk.*