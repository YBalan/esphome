# JK-BMS to MQTT Bridge with ESP32 & OLED Display

This project provides a robust solution for monitoring a **JK-BMS (Jikong)** using an **ESP32**. It retrieves data via Bluetooth (BLE) and transmits it to a local **Mosquitto MQTT** broker. The device features an integrated **OLED display** with power-saving logic and a **physical relay** control based on State of Charge (SoC).

## 🚀 Features
*   **BLE Integration:** Direct connection to JK-BMS (optimized for `JK02_32S` protocol).
*   **Local MQTT:** High-speed data transmission to a local server with password authentication.
*   **Smart OLED Display:** Shows real-time Voltage, Current, and SoC. Auto-off after 30 seconds to prevent burn-in.
*   **Physical Control:** Use the **BOOT button** (GPIO0) to wake the screen.
*   **Relay Automation:** Automatically toggles a relay on GPIO14 based on battery capacity (On < 200Ah, Off > 300Ah).
*   **Intelligent Calculations:** Provides a "Charging Time Remaining" sensor in a human-readable format (e.g., "2h 15m").
*   **Signal Diagnostics:** Monitors Wi-Fi RSSI and BLE RSSI for connection troubleshooting.

---

## 🛠 Hardware Requirements
*   **Microcontroller:** ESP32 Ideaspark with integrated 0.96" OLED (SSD1306 128x64).
*   **BMS:** JK-BMS (tested on model `JK_B2A8S20P`).
*   **Relay:** 5V Opto-isolated relay module connected to GPIO14.
*   **Power:** Reliable 5V USB power supply.

---

## 🐍 Installation & Build

This section explains how to install **Python 3.12.x**, set up a virtual environment, install **ESPHome**, and compile the firmware for your ESP32.

### 1. Install Python 3.12.x

#### Linux — Ubuntu / Debian
```bash
sudo apt update
sudo apt install -y python3.12 python3.12-venv python3.12-dev
```

#### Linux — Fedora / RHEL / CentOS
```bash
sudo dnf install -y python3.12
```

#### macOS — Homebrew
```bash
brew install python@3.12
```

#### Windows

**Option A — Official installer (recommended):**
1. Download the Python 3.12.x installer from [python.org/downloads](https://www.python.org/downloads/).
2. Run the installer, tick **"Add Python 3.12 to PATH"**, then click **Install Now**.

**Option B — pyenv-win:**
```powershell
pyenv install 3.12.10
pyenv global 3.12.10
```

---

### 2. Create a Virtual Environment

Using a virtual environment keeps ESPHome and its dependencies isolated from your system Python.

```bash
# Create the environment (run once, inside the jkbms_v2/ directory)
python3.12 -m venv .venv
```

Activate it before every session:

| Platform | Command |
|---|---|
| Linux / macOS | `source .venv/bin/activate` |
| Windows PowerShell | `.venv\Scripts\Activate.ps1` |
| Windows CMD | `.venv\Scripts\activate.bat` |

> **Tip:** Your shell prompt will change to show `(.venv)` when the environment is active.

---

### 3. Install ESPHome

With the virtual environment active, install ESPHome via pip:

```bash
pip install esphome
```

Verify the installation:

```bash
esphome version
```

> ESPHome automatically pulls in **PlatformIO** (the underlying build system) as a dependency — no separate installation is required.

---

### 4. Compile the Firmware

1. Make sure your virtual environment is active (see [step 2](#2-create-a-virtual-environment)).
2. Create your `secrets.yaml` in the `jkbms_v2/` directory (see the **ESPHome Configuration** section below) and fill in your credentials.
3. From the `jkbms_v2/` directory, run:

```bash
esphome compile jkbms_v2.yaml
```

ESPHome will download the required PlatformIO toolchains and the ESP-IDF framework on the first run — this may take several minutes. Subsequent builds are much faster.

**Output binary location:**
```
jkbms_v2/.esphome/build/jkbms-gateway-v2/.pioenvs/jkbms-gateway-v2/firmware.bin
```

4. *(Optional)* To compile **and** flash in one step (ESP32 connected via USB):

```bash
esphome run jkbms_v2.yaml
```

> **Note:** The first build downloads approximately 300 MB of toolchain data. Keep a stable internet connection during the initial compile.

---

## 🖥 Mosquitto MQTT Broker Setup (Windows)

To keep the system local and fast, Mosquitto is installed on a Windows PC.

### 1. Installation & User Creation
1. Download and install [Mosquitto for Windows](https://mosquitto.org/download/).
2. Open **Command Prompt (CMD)** as **Administrator**.
3. Create a password file and add a user (e.g., `yura`):
   ```cmd
   cd "C:\Program Files\mosquitto"
   mosquitto_passwd -c C:/mosquitto/passwords.txt yura
   ```
4. Enter your desired password when prompted.

### 2. Configure `mosquitto.conf`
Edit the configuration file (usually in `C:\Program Files\mosquitto\mosquitto.conf`). Add the following lines at the very end (avoid quotes if there are no spaces in the path):
```text
listener 1883
allow_anonymous false
password_file C:/mosquitto/passwords.txt
```

### 3. Windows Firewall & Permissions
*   **Firewall:** Create an "Inbound Rule" for **TCP port 1883** to allow the ESP32 to connect.
*   **File Permissions:** Right-click the `C:\mosquitto\passwords.txt` file -> Properties -> Security -> Edit -> Add. Type `SYSTEM` and `NETWORK_SERVICE` and grant "Full Control" to ensure the service can read the password file.
*   **Restart Service:** Open `services.msc`, find **Mosquitto Broker**, and restart it.

---

## ⚙️ ESPHome Configuration

### 1. `secrets.yaml`
Store your credentials securely in a separate file:
```yaml
wifi_ssid: "Your_SSID"
wifi_password: "Your_WiFi_Password"
mqtt_username: "yura"
mqtt_password: "Your_MQTT_Password"
mqtt_broker: "192.168.1.50"
```

### 2. Main YAML Highlights
Use `substitutions` at the top of your file for easy hardware management:

```yaml
substitutions:
  device_name: jkbms-gateway-v2
  bms_mac_address: "C8:47:80:28:71:3E"
  relay_control_pin: "14"
  display_on_time: "30s"
  bms_update_interval: "5s"
  total_capacity: "314.0"
  soc_low_limit: "200.0"
  soc_high_limit: "300.0"

esphome:
  name: ${device_name}
  on_boot:
    priority: -100
    then:
      - script.execute: display_timer

esp32:
  board: esp32dev
  framework:
    type: esp-idf # Recommended for stable BLE + WiFi coexistence

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  manual_ip: #optional
    static_ip: 192.168.1.253
    gateway: 192.168.1.1
    subnet: 255.255.255.0
    dns1: 8.8.8.8 # Fixes DNS Resolve Errors

mqtt:
  broker: !secret mqtt_broker # Your PC IP Address
  username: !secret mqtt_username
  password: !secret mqtt_password
  port: 1883
  topic_prefix: jkbms

i2c:
  sda: 21 # Change to 5 if using older Ideaspark boards
  scl: 22 # Change to 4 if using older Ideaspark boards
  scan: true
```

---

## 📊 MQTT Topic Map
Configure your mobile app (MQTT Dash / Home Assistant) using these topics:

### Telemetry Sensors
| Topic | Description |
|---|---|
| `jkbms/voltage` | Total battery voltage (V) |
| `jkbms/current` | Pack current in Amps (+ charging, − discharging) |
| `jkbms/soc` | Remaining capacity (Ah) |
| `jkbms/power` | Pack power (W) |
| `jkbms/t1` | Temperature sensor 1 (°C) |
| `jkbms/t2` | Temperature sensor 2 (°C) |
| `jkbms/min_c_v` | Minimum cell voltage (V) |
| `jkbms/max_c_v` | Maximum cell voltage (V) |
| `jkbms/delta` | Cell delta voltage (V) |
| `jkbms/wifi_rssi` | Wi-Fi signal strength (dBm) |
| `jkbms/ble_rssi` | BLE signal strength from BMS (dBm) |
| `jkbms/charging_time_hours` | Estimated time until full charge (hours, numeric) |

### Binary / Status
| Topic | Description |
|---|---|
| `jkbms/status/charging` | Charging active (true/false) |
| `jkbms/status/discharging` | Discharging active (true/false) |
| `jkbms/status/balancing` | Balancer active (true/false) |

### Relay Control
| Topic | Description |
|---|---|
| `jkbms/relay/state` | Current relay status (ON/OFF) |
| `jkbms/relay/set` | **Command topic** — send ON/OFF to toggle relay |

### JK BMS Switches
| State Topic | Command Topic | Description |
|---|---|---|
| `jkbms/switch/charge/state` | `jkbms/switch/charge/set` | Charging enabled |
| `jkbms/switch/discharge/state` | `jkbms/switch/discharge/set` | Discharging enabled |
| `jkbms/switch/balance/state` | `jkbms/switch/balance/set` | Balancer enabled |

### Text Sensors / Diagnostics
| Topic | Description |
|---|---|
| `jkbms/errors` | BMS error codes / text |
| `jkbms/charging_time_human` | Formatted charging time (e.g. "2h 15m" or "NA") |

---

## 📷 Images

![Hardware photo 1](images/photo_2026-03-22_11-06-48.jpg)

![Hardware photo 2](images/photo_2026-03-22_11-06-55.jpg)

---

## 🛠 Troubleshooting
*   **Error 0x8006 (SSL Timeout):** Avoid using port 8883. Use port 1883 without TLS to save ESP32 resources.
*   **Voltage shows 0.000V:** Your BMS likely uses a 300-byte data packet. Ensure `protocol_version` is set to `JK02_32S`.
*   **Display Communication Failed:** Check your I2C pins in the log. If (21, 22) fails, try (5, 4).
*   **DNS Resolve Error:** This happens when the ESP32 cannot find the MQTT server name. Always use a `manual_ip` block with `dns1: 8.8.8.8`.
