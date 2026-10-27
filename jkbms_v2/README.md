# JK-BMS Gateway v2

ESPHome firmware for an ESP32-based JK-BMS BLE gateway with MQTT publishing, OLED display, and SoC-based relay automation.

## Features

- JK-BMS BLE integration (`JK02_32S`) via external component
- MQTT + Home Assistant API integration
- OLED dashboard (SSD1306 128x64), auto-off timer, BOOT-button wake
- Configurable relay thresholds in Ah using HA/MQTT number entities
- Threshold defaults calculated from real BMS total capacity on boot
- User threshold values persist across reboots and are not overwritten after manual edits
- Integer threshold behavior (`step: 1`) with rounded values
- Charging-time and power split helper logic in `jkbms_v2.h`
- Wi-Fi RSSI and BLE RSSI diagnostics

## Current SoC Threshold Logic

- Real capacity source: `total_battery_capacity_setting` sensor (`BMS Total Capacity`)
- Defaults from substitutions:
  - `soc_low_limit_percentage: 0.8`
  - `soc_high_limit_percentage: 0.99`
  - initial values `251` / `311` Ah
- Boot script waits for total capacity sensor and initializes only if values are still unchanged defaults
- If either limit was edited by user, both restored values are kept

## Runtime Config Entities

### Writable limits (applied immediately)

- `BMS SoC Low Limit (Ah)`
- `BMS SoC High Limit (Ah)`

### Persisted text config (stored, rebuild required to apply)

- `BMS MAC Address Config`
- `MQTT Broker Config`
- `MQTT Username Config`
- `MQTT Password Config`

Note: BLE MAC and MQTT connection parameters are compile-time in ESPHome. Changing text config entities stores values, but firmware must be rebuilt/reflashed to apply them.

## Crash Diagnostics (no serial access)

The unit has no COM access in production, so resets are diagnosed from Home Assistant.

- `BMS Reset Reason`: why the last reset happened (`exception/panic`, `task watchdog`, ...).
- `BMS Heap *`, `BMS Loop Stack Free`, `BMS Loop Max Time`, `BMS Uptime`: put them in one history graph to tell leaks, stack overflow and loop stalls apart.
- `BMS Last Watchdog`: the ESPHome component the main loop was in when the task watchdog fired.
- `BMS Last Panic` / `BMS Last Panic Backtrace`: reason (`Fault`, `Abort` for `abort()`/asserts/failed allocations, `Task wdt`, `Interrupt wdt`), crashed core, PC, fault address, the id of the running ELF, and the code addresses of the call stack (both cores). Since ESPHome 2026.3 the crash itself is stored by ESPHome's built-in crash handler; `panic_capture.h` only replays it at boot through the logger (`on_message` in the YAML) and publishes it on these sensors. It must not define `__wrap_esp_panic_handler` or add `--wrap=esp_panic_handler`: ESPHome already does, and the link fails. The task name and uptime of the old implementation are no longer recorded.

A panic can only be decoded with the ELF of the exact build that was running, and every build overwrites `build/firmware.elf`. So each build archives its own ELF: the `elf_archive/` folder is a small local ESP-IDF component (`esp32: framework: components:` in the YAML) whose `project_include.cmake` adds a target that runs after the link and copies the ELF to **`elf-archive/`** in this folder (git-ignored, newest 20 kept) as `firmware-<date>_<time>-<id>.elf`. `<id>` is the first 8 hex digits of the ELF's SHA-256, the same value the device shows as `elf=` in `BMS Last Panic`. To decode a crash, paste the text of both panic sensors; the matching ELF is picked by that id and its hash is checked:

```cmd
python tools\decode_panic.py "<text of BMS Last Panic> <text of BMS Last Panic Backtrace>"
```

(`--elf <file>` overrides the choice.) A missing ELF or a failed copy only prints a warning during the build.

To check the capture works, enable the disabled-by-default `BMS Test Panic (abort)` or `BMS Test Panic (fault)` entity in Home Assistant and press it. The device reboots and both panic sensors must fill in. Disable the entity again afterwards.

## Important Topics

- Prefix: `jkbms`
- Telemetry examples:
  - `jkbms/total_capacity`
  - `jkbms/voltage`
  - `jkbms/current`
  - `jkbms/soc`
  - `jkbms/power`
  - `jkbms/charging_time_hours`
  - `jkbms/charging_time_human`
- Relay:
  - `jkbms/relay/state`
  - `jkbms/relay/set`

## Installation Guide

### 1. Prerequisites

- Python 3.12.x (recommended)
- ESPHome installed locally
- ESP32 board with USB cable
- Local MQTT broker (for example Mosquitto)

### 2. Install ESPHome

```cmd
pip install esphome
```

### 3. Prepare Files

1. Open the project folder `d:\esphome\jkbms_v2`.
2. Create or update `secrets.yaml` (example below).
3. Verify `bms_mac_address` matches your JK-BMS BLE MAC.

### 4. First Flash (USB)

```cmd
cd d:\esphome\jkbms_v2
esphome run jkbms_v2.yaml
```

### 5. Next Updates (OTA)

After the first USB flash, run the same command. ESPHome will offer OTA update when the node is reachable on Wi-Fi.

### 6. Optional Checks

Validate config only:

```cmd
esphome config jkbms_v2.yaml
```

View logs:

```cmd
esphome logs jkbms_v2.yaml
```

## Build / Flash

```cmd
cd d:\esphome\jkbms_v2
esphome run jkbms_v2.yaml
```

For config-only validation:

```cmd
esphome config jkbms_v2.yaml
```

## Required Secrets

Example `secrets.yaml` keys:

```yaml
wifi_ssid: "Your_SSID"
wifi_password: "Your_WiFi_Password"
mqtt_broker: "192.168.1.50"
mqtt_username: "user"
mqtt_password: "password"
bms_mac_address: "C8:47:80:28:71:3E"
```

## MQTT Broker Quick Example (Mosquitto)

`mosquitto.conf` minimum config:

```text
listener 1883
allow_anonymous false
password_file C:/mosquitto/passwords.txt
```
