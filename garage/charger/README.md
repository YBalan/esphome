# Generator Battery Charger

ESPHome configuration for the garage generator's idle-battery charger.

Project location in this repository: `garage/charger/`.

## Implemented behavior

- Primary target: ESP32 (`garage-charger-esp32-device.yaml`, `esp32dev` + esp-idf). The original ESP8266 build (`garage-charger-device.yaml`) is obsolete and kept only for reference.
- Board-specific GPIO pins live in `garage-charger-esp8266-pins.yaml` / `garage-charger-esp32-pins.yaml`; everything else (sensors, relay logic, display rendering) is shared via `garage-charger-charging.yaml` and `garage-charger-charging.h`.
- Built-in blue LED polarity is per-board (`gc_led_inverted`).
- INA226 monitors battery voltage only; DHT22 monitors ambient temperature.
- Hysteresis charging turns the relay on at/below a configurable low-voltage threshold and off at/above a configurable high-voltage threshold.
- Charging is also gated by a configurable safe temperature band.
- All four thresholds plus the display timeout are configurable from Home Assistant (`number` entities).
- 128x64 SSD1306 OLED shows charging status, battery voltage, temperature, and the configured start/stop voltage thresholds; auto-off after an idle timeout, wakes on charge start/stop or button press; shows the fallback AP name while in captive-portal mode.
- Built-in blue LED mirrors the charging relay state.
- `Last Charging Time` sensor shows the duration (`HH:mm`) of the last completed charging session, persisted across reboots.
- BOOT button wakes the display; holding it 10s+ triggers WiFi Forget.
- `Charging Feature` master switch lets an external automation force charging off regardless of thresholds; state persists across reboots.
- MQTT discovery is enabled alongside the native API, with `topic_prefix: ${device_name}`.

## MQTT topics

Config `number` entities are intentionally omitted here; this list covers only controls and sensors.

### Controls

- `garage-charger-esp32/switch/charging/command` and `garage-charger-esp32/switch/charging/state`
- `garage-charger-esp32/switch/charging_feature/command` and `garage-charger-esp32/switch/charging_feature/state`
- `garage-charger-esp32/switch/calibration/command` and `garage-charger-esp32/switch/calibration/state`
- `garage-charger-esp32/button/ina226_raw_dump/command`
- `garage-charger-esp32/button/wifi_forget/command`

### Sensors

- `garage-charger-esp32/sensor/wifi_rssi/state`
- `garage-charger-esp32/sensor/battery_voltage/state`
- `garage-charger-esp32/sensor/battery_percentage/state`
- `garage-charger-esp32/sensor/temperature/state`
- `garage-charger-esp32/sensor/humidity/state`
- `garage-charger-esp32/text_sensor/reset_reason/state`
- `garage-charger-esp32/text_sensor/wifi_name/state`
- `garage-charger-esp32/text_sensor/last_charging_time/state`
- `garage-charger-esp32/binary_sensor/is_charging/state`
