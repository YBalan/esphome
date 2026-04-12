# ESPHome Projects

A collection of ESPHome firmware configurations for various home automation hardware — boilers, BMS gateways, and remote displays — all integrated with Home Assistant via MQTT.

---

## Projects

| Folder | Device | Board | Description |
|--------|--------|-------|-------------|
| [`electrolux-boiler/`](electrolux-boiler/) | Electrolux EWH Boiler | ESP32 | UART control + monitoring, energy tracking, timer mode |
| [`electrolux-boiler-lcd/`](electrolux-boiler-lcd/) | Boiler LCD Dashboard | ESP32 | TFT display showing live boiler state from MQTT |
| [`electrolux-boiler-remote/`](electrolux-boiler-remote/) | Boiler Rotary Remote | ESP32 | TFT display + rotary encoder for local boiler control |
| [`jbdbms_v2/`](jbdbms_v2/) | JBD / XiaoXiang BMS | ESP32 | BLE → MQTT bridge with OLED display and relay automation |
| [`jkbms_v2/`](jkbms_v2/) | JK-BMS (Jikong) | ESP32 | BLE → MQTT bridge with OLED display and relay automation |

---

## electrolux-boiler

**Full UART integration with an Electrolux boiler** (EWH / Centurio series).

- Reads live temperature, power mode, BST, internal clock and timer from the boiler's UART bus every ~30 s
- Sends mode, temperature, clock-sync, timer and BST commands
- **Timer mode** — schedules a start time (`HH:MM`); boiler fires at that clock time
- **Energy tracking** — `Boiler Power` (W) + `Boiler Total Energy` (kWh, HA Energy Dashboard ready)
- **Temperature trend** — `Up` / `Down` / `Stable` via linear-regression slope over a 4-sample ring buffer; Up threshold is mode-dependent (faster heating = steeper slope required); Down threshold is fixed; only `Down` or `Stable` possible when not actively heating
- **Standalone monitoring** — ESP32 powered from 5 V USB independently; boiler's internal controller keeps running without mains grid, temperature still reported during grid outages
- MQTT discovery, web server, NTP clock sync, fallback AP

📁 [electrolux-boiler/README.MD](electrolux-boiler/README.MD)

---

## electrolux-boiler-lcd

**TFT display companion** for the Electrolux boiler — subscribes to MQTT topics published by `electrolux-boiler` and renders a live dashboard.

- Real-time current & target temperature, power mode, heating status
- **Temperature trend arrow** — ▲ green (Up), ▼ blue (Down), – grey (Stable), updated live via MQTT
- Adjustable brightness slider and landscape rotation toggle
- Auto-wake on MQTT update; screen-off timeout to prevent burn-in
- MQTT topic prefix: `boiler/lcd`

---

## electrolux-boiler-remote

**Standalone rotary-encoder remote** for the Electrolux boiler — a physical controller with a TFT display.

- Rotary encoder (GPIO32/33) adjusts target temperature; push confirms and sends command via MQTT
- TFT display with colour-coded status: current temp, target temp, mode, heating indicator
- Backlight PWM (GPIO4) with adjustable brightness
- Short press = wake / apply; long press = change power mode
- Wakes from sleep on any encoder turn; auto-dim after inactivity

---

## jbdbms_v2

**JBD / XiaoXiang / Jiabaida BMS** BLE → MQTT bridge.

- Voltage, current, SoC, power, cell delta, temperatures, charging time remaining
- Automated relay on GPIO14: ON below 160 Ah, OFF above 195 Ah
- 0.96″ OLED (SSD1306) — auto-off after 30 s, BOOT button wake
- MQTT topic prefix: `jbdbms/`

📁 [jbdbms_v2/README.md](jbdbms_v2/README.md)

---

## jkbms_v2

**JK-BMS (Jikong)** BLE → MQTT bridge (`JK02_32S` protocol).

- Voltage, current, SoC, power, cell delta, temperatures, MOSFET temp, cycle count
- Charging / discharging / balancing status; individual MOSFET switch control
- Automated relay on GPIO14: ON below 200 Ah, OFF above 300 Ah
- 0.96″ OLED (SSD1306) — auto-off after 30 s, BOOT button wake
- MQTT topic prefix: `jkbms/`

📁 [jkbms_v2/README.md](jkbms_v2/README.md)

---

## Common Setup

All projects share the same pattern:

1. **`secrets.yaml`** in each project folder (not committed):
   ```yaml
   wifi_ssid: "Your_SSID"
   wifi_password: "Your_WiFi_Password"
   mqtt_broker: "192.168.1.x"
   mqtt_username: "username"
   mqtt_password: "password"
   ```
2. **Flash** via USB first, then OTA for subsequent updates:
   ```cmd
   esphome run <project>/<config>.yaml
   ```
3. **Home Assistant** — all devices use MQTT discovery; entities appear automatically.
4. **Fallback AP** — if WiFi is unavailable, each device broadcasts a fallback access point for recovery.

---

## Links

- [ESPHome documentation](https://esphome.io)
- [dentra/esphome-ewh](https://github.com/dentra/esphome-ewh) — Electrolux boiler protocol reference
- [esphome-jbd-bms](https://github.com/syssi/esphome-jbd-bms) — JBD BMS external component
- [esphome-jk-bms](https://github.com/syssi/esphome-jk-bms) — JK BMS external component
