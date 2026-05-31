# Generator Controller Requirements

This document describes the ESPHome-based controller for the Dnipro-M GX-50iGR inverter generator.

## Goal

Build a generator controller on ESP32 that can monitor generator power, control the generator through relays, RF, and two mirrored high-torque choke servos, and expose the same actions in Home Assistant.

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
- RF receiver uses rc_switch dump mode, and RF transmitter is enabled for command emulation.
- LCD shows DHT values on line 1 and PZEM values on line 2.
- The two servos are mirrored and are used together to move the choke harder during stop sequences.
- Motor-hours are accumulated while the generator is running.
- Total energy counter is accumulated from measured power over time (kWh, total increasing).
- Last run timestamp is updated when generator state changes from running to stopped.
- Buttons 1 to 4 trigger their matching relay action and, if a learned RF code is stored, send that code as well.
- Each relay has configurable timeout in Home Assistant value fields; `-1` means infinite ON until manual OFF.

## Button Actions

1. Stop generator: sweep both mirrored servos and send learned RF stop when available.
2. Eco mode: toggle relay 2.
3. Start generator: trigger relay 3 and send learned RF start when available.
4. Garage rollete: trigger relay 4 and send learned RF rollete when available.
5. Settings: short press toggles display, long press toggles RF emulation.

## Home Assistant Control

The same physical-button actions are also exposed as ESPHome template buttons in Home Assistant.

Additional HA entities are exposed for automation and analytics:

- Per-relay timeout value fields (`Relay 1 Timeout` .. `Relay 4 Timeout`) with `-1` infinite mode.
- `Power HA Calc` (W) for HA-side power calculations.
- `Total Energy Counter` (kWh) as a total increasing energy source.
- `Last Run Timestamp` text sensor.

## Implementation Rules

- Keep lambdas in the companion header file.
- Use substitutions for GPIO, timings, and defaults.
- Avoid magic numbers and hard-coded strings where possible.
- Prefer const references in C++ helper code when practical.
- Keep YAML lambdas as thin adapters and place runtime logic in helper functions under `garage_generator_logic.h`.
