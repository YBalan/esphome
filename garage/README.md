# Garage Generator Controller

ESPHome configuration for an ESP32-based generator controller with power monitoring, mirrored choke servos, relay outputs, RF learn/transmit, LCD status, and Home Assistant integration.

## Implemented behavior (current)

- PZEM telemetry is polled every 2 seconds and published with staggered per-sensor throttles by priority.
- Running-state detection uses both voltage threshold and PZEM freshness checks.
- Stale PZEM values are actively zeroed (voltage/current/power/frequency/power factor) instead of staying `unknown`.
- `Total Energy` is integrated only while `Generator Running` is ON.
- `Motor Hours` is accumulated only while running and supports `Motor Hours Offset`.
- `Last Run Timestamp`, `Last Run Duration`, and `Current Run Duration` are exposed as text sensors.
- `Last Run Duration` and `Current Run Duration` are displayed as `HH:MM` (no seconds).
- `Current Run Duration` updates every 60 seconds and also refreshes on run-state transitions.
- If the device restarts while already running, current run duration starts from `00:00` and continues.
- Physical GPIO buttons are gated by `Physical Buttons Enabled` and default to OFF for safe bring-up.
- RF learn/capture is accepted only when learn mode is active.
- RF code text entities publish once at boot, then only on real changes (manual set or learn assignment).
- LCD update interval is slowed for lower runtime pressure.
- When fallback AP mode is active (captive portal), LCD line 2 shows `AP:<ssid>`.
- API batching currently uses ESPHome defaults (no custom `batch_delay` override).

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
- `Generator Use Servos`
- `Generator Servo On <Action> Action` switches
- `Generator Physical Buttons Enabled`
- Timeout number fields for Stop/Eco/Start/Rollete (`-1` = relay toggle behavior)
- `Generator Motor Hours Offset`
- `Generator Reset Motor Hours`
- `Generator Reset Total Energy`
- `Generator Reset All Counters`

### Runtime/diagnostic entities

- PZEM: voltage/current/power/energy/frequency/power factor
	- Published with staggered throttles (Voltage 2s, Current 3s, Power 5s, Power Factor 7s, Frequency 11s, Energy 29s)
- DHT11: temperature/humidity
- `Generator Running`
- `Generator Motor Hours`
- `Generator Total Energy`
- `Generator Last Run Timestamp`
- `Generator Last Run Duration`
- `Generator Current Run Duration`
- RF code text entities for Stop/Eco/Start/Rollete
- `Generator WiFi RSSI`

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
