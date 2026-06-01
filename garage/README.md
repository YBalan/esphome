# Garage Generator Controller

ESPHome configuration for an ESP32-based generator controller with power monitoring, RF control, mirrored choke servos, relays, LCD status display, and Home Assistant actions.

## What it does

- Monitors generator voltage, current, power, energy, frequency, and power factor through a PZEM-004T V3.0.
- Controls four active-low relays.
- Uses two mirrored 25 kg servos as a choke pair for stronger stop action.
- Learns and stores 433.92 MHz RF codes for buttons 1 to 4, then transmits them when RF Emulation is enabled.
- Reads a DHT11 sensor for temperature and humidity.
- Shows status on a 16x2 I2C LCD.
- Automatically turns the LCD backlight off after inactivity and wakes it on user actions.
- Tracks generator motor-hours.
- Tracks total accumulated energy (kWh) from runtime power integration.
- Exposes a dedicated power sensor for Home Assistant calculations.
- Stores last run timestamp when generator transitions from running to stopped.
- Exposes the physical-button actions as Home Assistant buttons.
- Exposes WiFi RSSI plus a Home Assistant button to reset WiFi.

## Current wiring summary

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

## Physical buttons

1. Stop generator
2. Eco mode
3. Start generator
4. Garage rollete
5. Settings / display control and RF learn mode

## Home Assistant buttons

The same actions are exposed in Home Assistant as template buttons:

- Action Stop
- Action Eco Toggle
- Action Start
- Action Rollete
- Action Settings Short
- Action Settings Long
- Action Stop Long
- Action Eco Long
- Action Start Long
- Action Rollete Long

## Home Assistant sensors and fields

- `Generator Stop Timeout`, `Generator Eco Timeout`, `Generator Start Timeout`, and `Generator Rollete Timeout` are value fields (`box` mode).
- Timeout `-1` means infinite (relay stays ON until manually turned OFF).
- `Generator Motor Hours` tracks total runtime hours.
- `Generator Total Energy Counter` tracks total increasing kWh.
- `Generator Power HA Calc` provides explicit power in watts for HA-side calculations.
- `Generator Last Run Timestamp` stores the last stop-time stamp.
- `Generator Stop RF Code`, `Generator Eco RF Code`, `Generator Start RF Code`, and `Generator RF Code Btn 4` show the stored RF codes.
- `Generator WiFi RSSI` reports signal strength.

## Files

- Main ESPHome config: [garage-generator.yaml](garage-generator.yaml)
- C++ helper logic: [garage_generator_logic.h](garage_generator_logic.h)
- Project requirements: [requirements.md](requirements.md)

## Build notes

- The config has been validated with `esphome config`.
- The firmware has been compiled successfully after resolving environment-specific build lock issues.
- GPIO5 and GPIO12 are strapping pins, so treat their wiring carefully.

## Notes

- DHT11 is currently used temporarily in the configuration.
- The servos are mirrored choke servos, not choke and throttle.
- Buttons 1 to 4 drive their matching relay actions first and can also send learned RF codes when one is stored and RF Emulation is enabled.
- Motor-hours are accumulated from runtime while the generator is considered running.
- RF learn mode is entered with a long press on Settings; captured RF codes are assigned by long-pressing the target button.
- Non-trivial runtime and display logic is centralized in the helper header.
