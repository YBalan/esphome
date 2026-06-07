# Generator Controller Requirements

This document describes the ESPHome-based controller for the Dnipro-M GX-50iGR inverter generator.

## Goal

Build a generator controller on ESP32 that can monitor generator power, control the generator through relays, RF, and two mirrored DS3230 Pro 30 kg 270 degree choke servos, and expose the same actions in Home Assistant.

## YAML Segregation

The configuration is intentionally split by responsibility and should stay that way:

- `garage-generator.yaml`: main device file for platform setup, network/API/logger, core globals, relays, physical buttons, shared HA buttons, core sensors, and action orchestration.
- `garage-generator-rf.yaml`: RF-only configuration for persisted RF raw storage, receiver/transmitter setup, RF learn/test/reset entities, RF raw text sensors, and RF scripts.
- `garage-generator-servos.yaml`: servo-only configuration for PWM outputs, servo entities, target angle inputs, persisted positions, and movement scripts.
- `garage-generator-mqtt.yaml`: MQTT broker config plus explicit custom command, telemetry, and retained state topics.
- `garage_generator_logic.h`: shared C++ helper logic for both the main file and split packages.

Documentation and future changes should preserve this separation instead of moving RF, servo, or MQTT details back into the main YAML file.

## Current Implementation Status

The following items are implemented and considered baseline behavior:

- PZEM-based telemetry over UART (`voltage/current/power/energy/frequency/power factor`).
- Running-state detection with PZEM freshness guard.
- Stale PZEM safety fallback that publishes zeroes instead of leaving stale `unknown` values.
- Runtime accounting:
	- `Motor Hours` accumulated only while running.
	- `Motor Hours Offset` supported.
	- `Last Run Start`, `Last Run Stop`, `Last Run Duration`, and `Current Run Duration` available.
	- `Last Run Duration` and `Current Run Duration` shown as `HH:MM`.
	- `Current Run Duration` updates every 60 seconds and is preserved across restart while the generator is already running.
- PZEM source is polled every 2 seconds, with staggered publish throttles by metric priority.
- RF raw text sensors publish on boot and then only when changed.
- Per-action RF raw reset buttons exist for Stop, Eco, Start, and Rollete, in addition to the full RF reset button.
- RF learn/assign/transmit workflow for actions 1..4.
- Mirrored servo control with per-action enable switches and global servo master switch.
- Servo manual controls for `Run Servos`, `Servos To Start`, `Servos To Stop`, and per-servo `Move ... To Angle` actions.
- A combined servo range text sensor is available in HA: `Generator Servo Start Stop Values`.
- Full HA action parity for physical actions through template buttons.
- Physical GPIO actions protected by `Physical Buttons Enabled` (default OFF).
- API batching and display pacing tuned for lower runtime pressure.
- MQTT default entity publishing is disabled (`topic_prefix: null`, `discovery: false`) and only explicit command/telemetry topics are used.
- MQTT behavior is controlled at runtime from HA with `Generator MQTT Enabled` (no compile-time toggle).

## Hardware

- ESP32 DevKit
- PZEM-004T V3.0 on hardware UART
- 4-channel active-low relay module
- 433.92 MHz ASK radio pair: SYN480R receiver and FS1000A/SYN115 transmitter
- Two DS3230 Pro 30 kg 270 degree servos used as a mirrored choke pair for stronger stop force
- DHT22 sensor used for temperature and humidity
- LCD 16x2 over I2C via PCF8574
- Five physical buttons
- Dual PSU setup with common ground

## GPIO Map

| Category | Device | GPIO | Notes |
| :--- | :--- | :--- | :--- |
| Relays | Relay 1 to 4 | 5, 18, 19, 23 | Active low |
| I2C | LCD SDA/SCL | 21, 22 | PCF8574 LCD backpack |
| UART2 | PZEM RX/TX | 16, 17 | RX on ESP32 connects to TX on PZEM |
| RF | Receiver / Transmitter | 27, 26 | Receiver needs level shifting or divider |
| Servos | Left Servo / Right Servo | 12, 13 | DS3230 Pro 30 kg 270 degree servos, left on GPIO12, right on GPIO13 |
| Sensor | DHT22 | 4 | Temperature/humidity sensor |
| Buttons | Button 1 to 5 | 14, 25, 32, 33, 35 | Button 5 uses GPIO35, external pull-up required |

## Electrical Notes

- Tie all grounds together.
- Use 5 V on JD-VCC for the relay board and 3.3 V on VCC when using opto-isolation.
- Use a divider on the RF receiver output before GPIO27.
- GPIO5 and GPIO12 are strapping pins, so their wiring must be handled carefully.
- Servo timing is configured for DS3230 Pro 270 degree travel: 50 Hz PWM with approximately 500us to 2500us pulse width (`2.5%` to `12.5%`).

## ESPHome Behavior

- PZEM is configured over UART on GPIO16/17.
- PZEM publish cadence is intentionally staggered to reduce API bursts:
	- Voltage 2s
	- Current 3s
	- Power 5s
	- Power Factor 7s
	- Frequency 11s
	- Energy 29s
- RF capture handler stores a pending raw payload only in learn mode.
- RF receive handling can be runtime-gated by `Generator RF Receiver Enabled` to reduce noise processing when not learning.
- Learned RF raw payloads can be assigned to buttons 1 to 4 from the physical buttons or the matching Home Assistant long-press buttons.
- Each assigned RF raw payload can be cleared independently with `Generator Reset RF Raw Stop/Eco/Start/Rollete`.
- Use RF is a master enable for transmitting learned RF raw payloads; it does not block relay actions.
- LCD shows DHT values on line 1 and keeps a manual line 2 page selected.
- Short Settings press cycles LCD line 2 between voltage/power and current/power factor.
- LCD backlight turns off automatically after inactivity.
- In fallback AP/captive mode, LCD line 2 shows `AP:<ssid>`.
- The two servos are mirrored and used together for stronger choke movement.
- Servo defaults are asymmetric because the left and right DS3230 Pro servos are mounted mirrored:
	- Left servo start/stop defaults: `150.0` -> `0.0`
	- Right servo start/stop defaults: `120.0` -> `270.0`
- Manual `Move Left/Right Servo To Angle` actions clamp by hardware max angle only.
- Saved Start/Stop values are exposed in one string: `Stop: Left/Right ; Start: Left/Right`.
- The combined Start/Stop text sensor is updated on boot and when Save Start/Save Stop is pressed.
- Motor-hours are accumulated only while the generator is running.
- Motor-hours offset can be manually set from HA.
- Last run start is updated when generator state changes from stopped to running.
- Last run stop is updated when generator state changes from running to stopped.
- Last run duration is captured on stop (`HH:MM`).
- Current run duration is minute-based (`HH:MM`) and updates every 60 seconds.
- If reboot happens during active run, current duration continues from the preserved run start timestamp.
- Buttons 1 to 4 trigger their matching relay action and, when Use RF is enabled, send the stored RF raw payload if one exists.
- Each relay has configurable timeout in Home Assistant value fields; `-1` makes action behave as relay toggle.
- MQTT command topics: `${device_name}/cmd/start`, `${device_name}/cmd/stop`, `${device_name}/cmd/eco`, `${device_name}/cmd/rollete`, `${device_name}/cmd/restart`, `${device_name}/cmd/wifi_reset`, `${device_name}/cmd/reset_rf_codes` (payload `PRESS`).
- MQTT telemetry topics (10s publish interval): `${device_name}/tele/voltage`, `${device_name}/tele/current`, `${device_name}/tele/power`, `${device_name}/tele/wifi_rssi`, `${device_name}/tele/current_run_duration`.
- MQTT relay/button status topics (published on relay state change, retained): `${device_name}/stat/stop`, `${device_name}/stat/eco`, `${device_name}/stat/start`, `${device_name}/stat/rollete` with payload `PRESSED` or `RELEASED`.
- MQTT command handling and custom topic publishing are active only when HA switch `Generator MQTT Enabled` is ON.

## Button Actions

1. Stop generator: sweep both mirrored servos and send learned RF stop when available.
2. Eco mode: toggle relay 2.
3. Start generator: trigger relay 3 and send learned RF start when available.
4. Garage rollete: trigger relay 4 and send learned RF rollete when available.
5. Settings: short press cycles LCD line 2 view, long press toggles RF learn mode.

## RF Learn Flow

- Long press Settings to enter RF learn mode.
- When an RF signal is received, the LCD shows the captured raw payload summary.
- Long press button 1, 2, 3, or 4 to store the captured raw payload for that action.
- The assigned RF raw payloads are persisted and exposed in Home Assistant diagnostic text sensors.
- Each stored RF raw payload can be cleared individually from Home Assistant without touching the other three assignments.

### Reliability notes for RF

- Long/noisy codes may require RF timing tuning.
- Overly permissive receiver settings can cause wrong capture and API backlog pressure.
- Tuning should be done incrementally and verified against both capture correctness and API stability.

## Servo Control

- A master `Use Servos` switch enables or disables servo usage.
- Per-action servo switches decide which actions actually run the servos.
- By default only the Stop action uses the servos.
- `Run Servos` performs the configured start-to-stop sequence for both servos.
- `Servos To Start` and `Servos To Stop` move both servos directly to the saved Start/Stop positions.
- `Move Left Servo To Angle` and `Move Right Servo To Angle` use target inputs and are limited only by `servo_max_angle_deg`.
- `Save Values As Start Position` and `Save Values As Stop Position` persist the current target angles as the saved range.

## Home Assistant Control

The same physical-button actions are also exposed as ESPHome template buttons in Home Assistant.

Additional HA controls are exposed for convenience:

- `Generator Use RF` toggles learned RF transmission.
- `Generator MQTT Enabled` toggles custom MQTT command/publish behavior at runtime.
- `Generator RF Receiver Enabled` toggles RF capture handling (useful to reduce noise outside learning).
- `Generator Use Servos` enables or disables servo control globally.
- `Generator Use Servo Action Stop`, `Eco`, `Start`, and `Rollete` control servo usage per action.
- `Generator Physical Buttons Enabled` controls whether physical GPIO button presses can trigger actions.
- `Generator Reset RF Codes` clears all stored RF raw payloads.
- `Generator Reset RF Raw Stop`, `Eco`, `Start`, and `Rollete` clear individual stored RF raw payloads.

Additional HA entities are exposed for automation and analytics:

- Per-action timeout value fields (`Stop Timeout`, `Eco Timeout`, `Start Timeout`, and `Rollete Timeout`) with `-1` toggle mode.
- `Motor Hours` and `Motor Hours Offset`.
- `Last Run Start` and `Last Run Stop` text sensors.
- `Last Run Duration` and `Current Run Duration` text sensors.
- `Generator Servo Start Stop Values` text sensor (format: `Stop: Left/Right ; Start: Left/Right`).
- `WiFi RSSI` sensor is available for connectivity diagnostics.

## Stability and Deployment Workflow (used in practice)

1. Reproduce and capture short logs around the fault.
2. Apply the smallest possible change (prefer rollback over broad refactor when unstable).
3. Validate with `esphome config` before upload.
4. Deploy via OTA when stable; use serial fallback when OTA is interrupted/reset.
5. Verify both symptom fix and adjacent risk areas:
	- RF learn correctness
	- API warnings (`Buffer full`, warning flags)
	- Running-state and duration counters

## Implementation Rules

- Keep lambdas in the companion header file.
- Use substitutions for GPIO, timings, and defaults.
- Avoid magic numbers and hard-coded strings where possible.
- Prefer const references in C++ helper code when practical.
- Keep YAML lambdas as thin adapters and place runtime logic in helper functions under `garage_generator_logic.h`.
