# Garage Generator Controller

ESPHome configuration for an ESP32-based generator controller with power monitoring, mirrored choke servos, relay outputs, RF learn/transmit, LCD status, and Home Assistant integration.

## Screenshots

![Garage Generator screenshot 1](images/Screenshot%202026-06-05%20175842.png)

![Garage Generator screenshot 2](images/Screenshot%202026-06-05%20175930.png)

![Garage Generator screenshot 3](images/Screenshot%202026-06-05%20175944.png)

![Garage Generator screenshot 4](images/Screenshot%202026-06-05%20175956.png)

## Implemented behavior (current)

- PZEM telemetry is polled every 2 seconds and published with staggered per-sensor throttles by priority.
- Running-state detection uses both voltage threshold and PZEM freshness checks.
- Stale PZEM values are actively zeroed (voltage/current/power/frequency/power factor) instead of staying `unknown`.
- `Motor Hours` is accumulated only while running and supports `Motor Hours Offset`.
- `Last Run Start`, `Last Run Stop`, `Last Run Duration`, and `Current Run Duration` are exposed as text sensors.
- `Last Run Duration` and `Current Run Duration` are displayed as `HH:MM` (no seconds).
- `Current Run Duration` updates every 60 seconds and also refreshes on run-state transitions.
- If the device restarts while already running, current run duration is resumed from the preserved run start timestamp.
- Physical GPIO buttons are gated by `Physical Buttons Enabled` and default to OFF for safe bring-up.
- RF learn/capture is accepted only when learn mode is active.
- RF receive handling is controlled by `Generator RF Receiver Enabled` (OFF blocks capture handling to reduce noise).
- RF code text entities publish once at boot, then only on real changes (manual set or learn assignment).
- LCD update interval is slowed for lower runtime pressure.
- LCD line 2 no longer rotates automatically; short Settings press cycles line 2 view (V/P <-> I/PF).
- When fallback AP mode is active (captive portal), LCD line 2 shows `AP:<ssid>`.
- API batching is intentionally limited (`batch_delay: 250ms`) to reduce burst pressure.
- MQTT auto-published entities are disabled (`topic_prefix: null`, `discovery: false`); only explicit `cmd` and `tele` topics are used.
- MQTT behavior is runtime-controlled from HA by `Generator MQTT Enabled` (no recompile required).

## Hardware summary

| Function | GPIO |
| :--- | :--- |
| Relay 1 | 5 |
| Relay 2 | 18 |
| Relay 3 | 19 |
| Relay 4 | 23 |
| LCD SDA | 21 |
| LCD SCL | 22 |
| PZEM RX | 16 |
| PZEM TX | 17 |
| RF RX | 27 |
| RF TX | 26 |
| Servo 1 | 12 |
| Servo 2 | 13 |
| DHT | 4 |
| Button 1 | 14 |
| Button 2 | 25 |
| Button 3 | 32 |
| Button 4 | 33 |
| Button 5 | 35 |

## Home Assistant actions and entities

### Action buttons

- Action Stop / Stop Long
- Action Eco On / Eco Long
- Action Start / Start Long
- Action Rollete / Rollete Long
- Action Settings Short / Settings Long

### Config and operational entities

- `Generator Use RF`
- `Generator MQTT Enabled`
- `Generator Use Servos`
- `Generator Servo On <Action> Action` switches
- `Generator Physical Buttons Enabled`
- Timeout number fields for Stop/Eco/Start/Rollete (`-1` = relay toggle behavior)
- `Generator Motor Hours Offset`
- `Generator Reset Motor Hours`
- `Generator Reset All Counters`

### Runtime/diagnostic entities

- PZEM: voltage/current/power/energy/frequency/power factor
	- Published with staggered throttles (Voltage 2s, Current 3s, Power 5s, Power Factor 7s, Frequency 11s, Energy 29s)
- DHT22: temperature/humidity
- `Generator Running`
- `Generator Motor Hours`
- `Generator Last Run Start`
- `Generator Last Run Stop`
- `Generator Last Run Duration`
- `Generator Current Run Duration`
- RF code text entities for Stop/Eco/Start/Rollete
- `Generator WiFi RSSI`

## MQTT contract

The device uses manual MQTT topics only.

- Runtime enable/disable:
	- `Generator MQTT Enabled` = ON: command handling + telemetry/status publishing active.
	- `Generator MQTT Enabled` = OFF: MQTT commands are ignored and custom telemetry/status publishing is paused.

- Commands:
	- `${device_name}/cmd/start` payload `PRESS`
	- `${device_name}/cmd/stop` payload `PRESS`
	- `${device_name}/cmd/eco` payload `PRESS`
	- `${device_name}/cmd/rollete` payload `PRESS`
	- `${device_name}/cmd/restart` payload `PRESS`
	- `${device_name}/cmd/wifi_reset` payload `PRESS`
	- `${device_name}/cmd/reset_rf_codes` payload `PRESS`
- Telemetry (published every 10 seconds when MQTT is connected):
	- `${device_name}/tele/voltage`
	- `${device_name}/tele/current`
	- `${device_name}/tele/power`
	- `${device_name}/tele/wifi_rssi`
	- `${device_name}/tele/current_run_duration`
- Relay/button state topics (published on every relay change, retained):
	- `${device_name}/stat/stop` -> `PRESSED`/`RELEASED`
	- `${device_name}/stat/eco` -> `PRESSED`/`RELEASED`
	- `${device_name}/stat/start` -> `PRESSED`/`RELEASED`
	- `${device_name}/stat/rollete` -> `PRESSED`/`RELEASED`

## RF learn flow

1. Enter learn mode from Settings long action (physical or HA action button).
2. Send remote RF signal.
3. Assign captured code by long action for button 1, 2, 3, or 4.
4. Use RF action transmission occurs only when `Use RF` is ON and a code is assigned.

## Operational notes from recent work

- If logs show `remote_receiver took a long time` together with `api.connection Buffer full`, treat it as receiver/API pressure first.
- Keep RF timing parameters conservative when tuning; aggressive decode settings can improve sensitivity but hurt correctness/stability.
- If OTA upload is unstable, use serial flashing fallback.
- GPIO5 and GPIO12 are strapping pins; treat external pull-up/down wiring carefully.

## Files

- Main config: [garage-generator.yaml](garage-generator.yaml)
- Runtime helper logic: [garage_generator_logic.h](garage_generator_logic.h)
- Requirements/spec notes: [requirements.md](requirements.md)

## Wiring Images

Additional wiring and hardware photos:

- ![Wiring photo 1](images/IMG_20260601_113656.jpg)
- ![Wiring photo 2](images/IMG_20260605_150307.jpg)
- ![Wiring photo 3](images/IMG_20260605_154435.jpg)
- ![Wiring photo 4](images/IMG_20260605_154441.jpg)
- ![Wiring photo 5](images/IMG_20260605_154445.jpg)
- ![Wiring photo 6](images/IMG_20260605_154452.jpg)
- ![Wiring photo 7](images/IMG_20260605_154841.jpg)
- ![Wiring photo 8](images/IMG_20260605_154845.jpg)
- ![Wiring photo 9](images/IMG_20260605_154904.jpg)
- ![Wiring photo 10](images/IMG_20260605_154916.jpg)
- ![Wiring photo 11](images/IMG_20260605_155123.jpg)
