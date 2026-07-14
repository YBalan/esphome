# ESPHome Projects

A collection of ESPHome firmware configurations for home automation hardware: boiler controllers, BMS gateways, and room-light automation.

---

## Projects

| Folder | Device | Board | Description |
|--------|--------|-------|-------------|
| [`electrolux-boiler/`](electrolux-boiler/) | Electrolux EWH Boiler | ESP32 | UART control + monitoring, energy tracking, timer mode |
| [`electrolux-boiler-lcd/`](electrolux-boiler-lcd/) | Boiler LCD Dashboard | ESP32 | TFT display showing live boiler state from MQTT |
| [`electrolux-boiler-remote/`](electrolux-boiler-remote/) | Boiler Rotary Remote | ESP32 | TFT display + rotary encoder for local boiler control |
| [`jbdbms_v2/`](jbdbms_v2/) | JBD / XiaoXiang BMS | ESP32 | BLE → MQTT bridge with OLED display and relay automation |
| [`jkbms_v2/`](jkbms_v2/) | JK-BMS (Jikong) | ESP32 | BLE → MQTT bridge with OLED display and relay automation |
| [`garage/generator/`](garage/generator/) | Garage Generator Controller | ESP32 | Generator control + PZEM telemetry + RF learn/transmit + mirrored servos |
| [`wc-lights/`](wc-lights/) | WC + Bath Lighting Controller | ESP32 | Door-driven light automation, RGB status strip, visit statistics |
| [`ai-helper/`](ai-helper/) | AI Helper (Semaphore + Mouse Mover) | ESP32 | RGB/buzzer status semaphore for coding-agent hooks, servo-driven mouse mover |

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
- Total capacity comes from BMS settings (`nominal_capacity`) and is exposed as read-only telemetry
- Configurable SoC relay thresholds (`JBD SoC Low Limit (Ah)`, `JBD SoC High Limit (Ah)`) persisted across reboots
- Boot-time smart init calculates rounded default thresholds from total capacity, but preserves user changes
- 0.96″ OLED (SSD1306) — auto-off after 30 s, BOOT button wake
- Helper lambdas extracted to `jbdbms_v2.h` (display lambda remains in YAML)
- Config text entities for BMS MAC and MQTT broker/user/pass are stored in HA; applying them still requires rebuild/reflash
- MQTT topic prefix: `jbdbms/`

📁 [jbdbms_v2/README.md](jbdbms_v2/README.md)

---

## jkbms_v2

**JK-BMS (Jikong)** BLE → MQTT bridge (`JK02_32S` protocol).

- Voltage, current, SoC, power, cell delta, temperatures, MOSFET temp, cycle count
- Total capacity comes from BMS settings (`total_battery_capacity_setting`) and is exposed as read-only telemetry
- Charging / discharging / balancing status; individual MOSFET switch control
- Configurable SoC relay thresholds (`BMS SoC Low Limit (Ah)`, `BMS SoC High Limit (Ah)`) persisted across reboots
- Boot-time smart init calculates rounded default thresholds from total capacity, but preserves user changes
- 0.96″ OLED (SSD1306) — auto-off after 30 s, BOOT button wake
- Config text entities for BMS MAC and MQTT broker/user/pass are stored in HA; applying them still requires rebuild/reflash
- MQTT topic prefix: `jkbms/`

📁 [jkbms_v2/README.md](jkbms_v2/README.md)

---

## wc-lights

**WC and Bath smart lighting controller** with occupancy and visit analytics.

- Automatic day/night brightness control per room
- Door-open timeout logic to avoid false occupancy
- RGB status strip for inside status and post-visit flash
- WC inside blink reminder (configurable interval and blink count)
- Visit counters and duration stats persisted across reboots
- Endstop mode (NC/NO) configurable from Home Assistant
- Works offline with captive portal and local GPIO logic

📁 [wc-lights/README.md](wc-lights/README.md)

---

## garage/generator

**Garage generator controller** with relay actions, RF learn/transmit support, mirrored choke servos, and runtime tracking.

- PZEM telemetry with stale-data guard and staggered publish strategy
- Running-state detection with runtime counters (`Motor Hours`, last/current run duration)
- `Current Run Duration` is preserved across restart when reboot happens during an active run
- MQTT uses explicit command/telemetry topics only (`topic_prefix: null`, `discovery: false`)

📁 [garage/generator/README.md](garage/generator/README.md)

---

## ai-helper

**Coding-agent desk companion** — a status semaphore plus an optional mouse mover, sharing one ESPHome codebase.

- **Semaphore** — RGB LED + buzzer shows agent status (`Idle`/`Reasoning`/`Done`/`Need User Action`/`Allow Action`/`Custom1`/`Custom2`), each fully configurable (color, LED slot, brightness, buzzer) from Home Assistant
- Status can be set via HA select entity, native API action, or a plain HTTP button-press endpoint — the latter is used to wire up Claude Code and GitHub Copilot hooks so the agent's live status shows up on the semaphore
- **Usage gauge** — a `Usage Remaining` % readout plus a `Usage Source` label showing which tool set it. Claude Code posts its **real plan-session usage** (derived from Anthropic's rate-limit headers) automatically at reasoning start and on `Stop`; Copilot posts best-effort values and updates both source and remaining at reasoning start and at done/stop
- **Mouse mover** — servo sweep on a timer with LCD menu and physical buttons; random angle/step/delay/period optionally seeded from random.org, with local fallback
- Buildable as one combined device (`ai-helper.yaml`) or two standalone devices (`semaphore-device.yaml`, `mouse-mover-device.yaml`)

📁 [ai-helper/README.md](ai-helper/README.md)

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
   Garage generator example from repository root:
   ```cmd
   esphome run garage/generator/garage-generator.yaml
   ```
3. **Home Assistant / MQTT** — most devices use MQTT discovery; the garage generator project uses manual MQTT topics with discovery disabled.
4. **Fallback AP** — if WiFi is unavailable, each device broadcasts a fallback access point for recovery.

---

## Links

- [ESPHome documentation](https://esphome.io)
- [dentra/esphome-ewh](https://github.com/dentra/esphome-ewh) — Electrolux boiler protocol reference
- [esphome-jbd-bms](https://github.com/syssi/esphome-jbd-bms) — JBD BMS external component
- [esphome-jk-bms](https://github.com/syssi/esphome-jk-bms) — JK BMS external component
