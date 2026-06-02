# Generator Controller Requirements

This document describes the ESPHome-based controller for the Dnipro-M GX-50iGR inverter generator.

## Goal

Build a generator controller on ESP32 that can monitor generator power, control the generator through relays, RF, and two mirrored high-torque choke servos, and expose the same actions in Home Assistant.

## Current Implementation Status

The following items are implemented and considered baseline behavior:

- PZEM-based telemetry over UART (`voltage/current/power/energy/frequency/power factor`).
- Running-state detection with PZEM freshness guard.
- Stale PZEM safety fallback that publishes zeroes instead of leaving stale `unknown` values.
- Runtime accounting:
	- `Motor Hours` accumulated only while running.
	- `Motor Hours Offset` supported.
	- `Total Energy` accumulated only while running.
	- `Last Run Timestamp`, `Last Run Duration`, and `Current Run Duration` available.
- RF learn/assign/transmit workflow for actions 1..4.
- Mirrored servo control with per-action enable switches and global servo master switch.
- Full HA action parity for physical actions through template buttons.
- Physical GPIO actions protected by `Physical Buttons Enabled` (default OFF).
- API batching and display pacing tuned for lower runtime pressure.

## Hardware

- ESP32 DevKit
- PZEM-004T V3.0 on hardware UART
- 4-channel active-low relay module
- 433.92 MHz ASK radio pair: SYN480R receiver and FS1000A/SYN115 transmitter
- Two 25 kg servos used as a mirrored choke pair for stronger stop force
- DHT11 sensor temporarily used for temperature and humidity
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
| Servos | Servo 1 / Servo 2 | 12, 13 | Mirrored choke servos |
| Sensor | DHT11 | 4 | Temporary sensor model |
| Buttons | Button 1 to 5 | 14, 25, 32, 33, 35 | Button 5 uses GPIO35, external pull-up required |

## Electrical Notes

- Tie all grounds together.
- Use 5 V on JD-VCC for the relay board and 3.3 V on VCC when using opto-isolation.
- Use a divider on the RF receiver output before GPIO27.
- GPIO5 and GPIO12 are strapping pins, so their wiring must be handled carefully.

## ESPHome Behavior

- PZEM is configured over UART on GPIO16/17.
- RF capture handler stores a pending code only in learn mode.
- Learned RF codes can be assigned to buttons 1 to 4 from the physical buttons or the matching Home Assistant long-press buttons.
- Use RF is a master enable for transmitting learned RF codes; it does not block relay actions.
- LCD shows DHT values on line 1 and alternates line 2 between voltage/power and current/power factor.
- LCD backlight turns off automatically after inactivity and short Settings press only wakes the display.
- The two servos are mirrored and used together for stronger choke movement.
- Motor-hours are accumulated only while the generator is running.
- Motor-hours offset can be manually set from HA.
- Total energy is accumulated only while the generator is running (kWh, total increasing).
- Last run timestamp is updated when generator state changes from running to stopped.
- Last run duration is captured on stop.
- Current run duration is updated from state/sensor events.
- Buttons 1 to 4 trigger their matching relay action and, when Use RF is enabled, send the stored RF code if one exists.
- Each relay has configurable timeout in Home Assistant value fields; `-1` means infinite ON until manual OFF.

## Button Actions

1. Stop generator: sweep both mirrored servos and send learned RF stop when available.
2. Eco mode: toggle relay 2.
3. Start generator: trigger relay 3 and send learned RF start when available.
4. Garage rollete: trigger relay 4 and send learned RF rollete when available.
5. Settings: short press wakes the display, long press toggles RF learn mode.

## RF Learn Flow

- Long press Settings to enter RF learn mode.
- When an RF signal is received, the LCD shows the captured code.
- Long press button 1, 2, 3, or 4 to store the captured code for that action.
- The assigned codes are persisted and also exposed in Home Assistant text fields.

### Reliability notes for RF

- Long/noisy codes may require RF timing tuning.
- Overly permissive receiver settings can cause wrong capture and API backlog pressure.
- Tuning should be done incrementally and verified against both capture correctness and API stability.

## Servo Control

- A master `Use Servos` switch enables or disables servo usage.
- Per-action servo switches decide which actions actually run the servos.
- By default only the Stop action uses the servos.

## Home Assistant Control

The same physical-button actions are also exposed as ESPHome template buttons in Home Assistant.

Additional HA controls are exposed for convenience:

- `Generator Use RF` toggles learned RF transmission.
- `Generator Use Servos` enables or disables servo control globally.
- `Generator Use Servo Action Stop`, `Eco`, `Start`, and `Rollete` control servo usage per action.
- `Generator Physical Buttons Enabled` controls whether physical GPIO button presses can trigger actions.

Additional HA entities are exposed for automation and analytics:

- Per-action timeout value fields (`Stop Timeout`, `Eco Timeout`, `Start Timeout`, and `Rollete Timeout`) with `-1` infinite mode.
- `Total Energy` (kWh) as a total increasing energy source.
- `Motor Hours` and `Motor Hours Offset`.
- `Last Run Timestamp` text sensor.
- `Last Run Duration` and `Current Run Duration` text sensors.
- `WiFi RSSI` sensor and a `Reset WiFi` button are available for connectivity management.

## Stability and Deployment Workflow (used in practice)

1. Reproduce and capture short logs around the fault.
2. Apply the smallest possible change (prefer rollback over broad refactor when unstable).
3. Validate with `esphome config` before upload.
4. Deploy via OTA when stable; use serial fallback when OTA is interrupted/reset.
5. Verify both symptom fix and adjacent risk areas:
	- RF learn correctness
	- API warnings (`Buffer full`, warning flags)
	- Running-state, energy, and duration counters

## Implementation Rules

- Keep lambdas in the companion header file.
- Use substitutions for GPIO, timings, and defaults.
- Avoid magic numbers and hard-coded strings where possible.
- Prefer const references in C++ helper code when practical.
- Keep YAML lambdas as thin adapters and place runtime logic in helper functions under `garage_generator_logic.h`.
