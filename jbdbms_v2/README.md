# JBD-BMS Gateway v2

ESPHome firmware for an ESP32-based JBD/XiaoXiang BLE gateway with MQTT publishing, OLED display, and SoC-based relay automation.

## Features

- JBD BLE integration via `esphome-jbd-bms`
- MQTT + Home Assistant API integration
- OLED dashboard (SSD1306 128x64), auto-off timer, BOOT-button wake
- Configurable relay thresholds in Ah using HA/MQTT number entities
- Threshold defaults calculated from real JBD nominal capacity on boot
- User threshold values persist across reboots and are not overwritten after manual edits
- Integer threshold behavior (`step: 1`) with rounded values
- Non-display lambdas extracted to `jbdbms_v2.h`
- Wi-Fi RSSI and BLE RSSI diagnostics

## Current SoC Threshold Logic

- Real capacity source: `nominal_capacity` sensor (`JBD Total Capacity`)
- Defaults from substitutions:
    - `soc_low_limit_percentage: 0.8`
    - `soc_high_limit_percentage: 0.975`
    - initial values `160` / `195` Ah
- Boot script waits for total capacity sensor and initializes only if values are still unchanged defaults
- If either limit was edited by user, both restored values are kept

## Runtime Config Entities

### Writable limits (applied immediately)

- `JBD SoC Low Limit (Ah)`
- `JBD SoC High Limit (Ah)`

### Persisted text config (stored, rebuild required to apply)

- `JBD BMS MAC Address Config`
- `JBD MQTT Broker Config`
- `JBD MQTT Username Config`
- `JBD MQTT Password Config`

Note: BLE MAC and MQTT connection parameters are compile-time in ESPHome. Changing text config entities stores values, but firmware must be rebuilt/reflashed to apply them.

## Connectivity / Stability Settings

Kept in step with `jkbms_v2` (same ESP32 + BLE + WiFi + MQTT load, same crash history):

- WiFi: `power_save_mode: NONE`, `reboot_timeout: 0s`, `post_connect_roaming: false` (roam scans share the 2.4 GHz radio with BLE and cause multi-second BLE stalls).
- API `reboot_timeout: 5min`; MQTT `reboot_timeout: 0s`, `keepalive: 30s`, `discovery: false` (the discovery JSON for every entity ran jkbms_v2 out of heap; HA uses the native API).
- BLE tracker: `software_coexistence: false`. Scanning stays **passive** here (`active: false`, a deliberate JBD choice that reduces radio airtime); jkbms_v2 uses active scanning.
- Task watchdog 30 s (`CONFIG_ESP_TASK_WDT_TIMEOUT_S`), main loop task stack 12288 B (`loop_task_stack_size`).

## Crash Diagnostics (no serial access)

If the unit has no COM access, resets are diagnosed from Home Assistant. Same entities as `jkbms_v2`, named `JBD ...`; see the jkbms_v2 README for the long form.

- `JBD Reset Reason`: why the last reset happened (`exception/panic`, `task watchdog`, ...).
- `JBD Heap *`, `JBD Loop Stack Free`, `JBD Loop Max Time`, `JBD Uptime`: put them in one history graph to tell leaks, stack overflow and loop stalls apart. `JBD Loop Stack Free` also shows whether the larger loop stack is needed.
- `JBD Last Watchdog`, `JBD Last Watchdog Backtrace`, `JBD Last Watchdog BT Host Tasks`, `JBD Last Watchdog BT HCI Tasks`: the ESPHome component the main loop was in when the task watchdog fired, and the saved call stacks of `loopTask` and the Bluetooth tasks. Recorded by the watchdog interrupt hook in `wdt_forensics.h`.
- `JBD Last Panic` / `JBD Last Panic Backtrace`: reason, crashed core, PC, fault address, ELF id (`elf=<id>`) and code addresses of the last panic. ESPHome's built-in crash handler stores it; `panic_capture.h` replays it at boot through `logger: on_message` and publishes it. Never add `--wrap=esp_panic_handler` or define `__wrap_esp_panic_handler`: ESPHome already does, and the link fails.

Every build copies its ELF to `elf-archive/` (git-ignored, newest 20 kept) through the `elf_archive/` local ESP-IDF component, because a panic can only be decoded with the ELF of the build that was running. Paste the text of both panic sensors:

```cmd
python tools\decode_panic.py "<text of JBD Last Panic> <text of JBD Last Panic Backtrace>"
```

(`--elf <file>` overrides the choice.) Only builds flashed after this was added can be decoded.

To check the capture, enable the disabled-by-default `JBD Test Panic (abort)`, `JBD Test Panic (fault)` or `JBD Test Watchdog (block loop)` entity in Home Assistant and press it. The device resets on purpose (the watchdog test blocks the main loop for 40 s, so the relay and BMS polling stop). Disable the entity again afterwards.

## Important Topics

- Prefix: `jbdbms`
- Telemetry examples:
    - `jbdbms/total_capacity`
    - `jbdbms/voltage`
    - `jbdbms/current`
    - `jbdbms/soc`
    - `jbdbms/power`
    - `jbdbms/charging_time_hours`
    - `jbdbms/charging_time_human`
- Relay:
    - `jbdbms/relay/state`
    - `jbdbms/relay/set`

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

1. Open the project folder `d:\esphome\jbdbms_v2`.
2. Create or update `secrets.yaml` (example below).
3. Verify `bms_mac_address` matches your JBD BMS BLE MAC.

### 4. First Flash (USB)

```cmd
cd d:\esphome\jbdbms_v2
esphome run jbdbms_v2.yaml
```

### 5. Next Updates (OTA)

After the first USB flash, run the same command. ESPHome will offer OTA update when the node is reachable on Wi-Fi.

### 6. Optional Checks

Validate config only:

```cmd
esphome config jbdbms_v2.yaml
```

View logs:

```cmd
esphome logs jbdbms_v2.yaml
```

## Build / Flash

```cmd
cd d:\esphome\jbdbms_v2
esphome run jbdbms_v2.yaml
```

For config-only validation:

```cmd
esphome config jbdbms_v2.yaml
```

## Required Secrets

Example `secrets.yaml` keys:

```yaml
wifi_ssid: "Your_SSID"
wifi_password: "Your_WiFi_Password"
mqtt_broker: "192.168.1.50"
mqtt_username: "user"
mqtt_password: "password"
bms_mac_address: "A5:C2:37:05:69:89"
```

## MQTT Broker Quick Example (Mosquitto)

`mosquitto.conf` minimum config:

```text
listener 1883
allow_anonymous false
password_file C:/mosquitto/passwords.txt
```