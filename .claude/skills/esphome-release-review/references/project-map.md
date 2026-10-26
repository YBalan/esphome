# Project map: devices, components, external sources

Used by `esphome-release-review` to decide which changelog entries matter. Every device
also gets the shared baseline from its `<device>-common.yaml` (wifi + AP/captive portal,
ota, logger, time/sntp, debug + reset-reason, wifi signal/info). Keep this file current
when a device or component is added or removed.

## Devices

| Folder | Entry file(s) | Key components | External source |
| --- | --- | --- | --- |
| `jbdbms_v2` | `jbdbms_v2.yaml` | `ble_client`, `esp32_ble_tracker`, `jbd_bms_ble`, `ssd1306_i2c`, `mqtt`, `api`, `interval`, `script`, `preferences`, `factory_reset` | `github://syssi/esphome-jbd-bms@main` |
| `jkbms_v2` | `jkbms_v2.yaml` | `ble_client`, `esp32_ble_tracker`, `jk_bms_ble` (BLE, not UART), `ssd1306_i2c`, `mqtt`, `api`, `interval`, `script`, `preferences`, `factory_reset`, `panic_capture.h` (adapter over the built-in crash handler) | `github://syssi/esphome-jk-bms@main` |
| `grid-meter` | `grid-meter-device.yaml` (+ `-common`, `-pzem`) | `uart`, `pzemac` (ESPHome `modbus` layer), `sntp` | none |
| `ai-helper` | `ai-helper.yaml`, `mouse-mover-device.yaml`, `semaphore-device.yaml` (+ common/defaults) | `web_server`, `http_request`, `lcd_pcf8574`, `esp32_rmt_led_strip`, `ledc`, `servo`, `partition` OTA, `light`, `select`, `text` | none |
| `electrolux-boiler` | `electrolux-boiler.yaml` | `uart`, `mqtt`, `api`, `interval`, `factory_reset` | none |
| `electrolux-boiler-lcd` | `electrolux-boiler-LCD.yaml` (`test.yaml` is a scratch file) | `st7789v` (`spi`), `light` monochromatic/`ledc`, `mqtt_subscribe`, `web_server` | none |
| `electrolux-boiler-remote` | `electrolux-boiler-remote.yaml` | `st7789v`, `rotary_encoder`, `mqtt_subscribe`, `light` | none |
| `wc-lights` | `wc-lights.yaml` | `esp32_rmt_led_strip`, `ledc`, `light`, `preferences`, `factory_reset` | none |
| `garage/charger` | `garage-charger-esp32-device.yaml` | `ssd1306_i2c`, `dht`, `ina226`, `mqtt`, `web_server`, `preferences` | none |
| `garage/generator` | `garage-generator.yaml` (+ `-mqtt`, `-rf`, `-servos`, `-pzem`) | `remote_receiver`/`remote_transmitter`, `servo`, `lcd_pcf8574`, `dht`, `uart` + explicit `modbus` hub + `pzemac` (in `-pzem`), `mqtt` | none |
| `RS232/logicpower` | `logicpower-ups-device.yaml`, `-pip8048`, `-powermanager`, `-protocol-test` | `uart`, `pipsolar`, `pip8048` | `github://syssi/esphome-pipsolar@322e2b2...` (pinned commit) |

Not part of the review: `esphome-jbd-bms/` is a vendored copy of the upstream repo (its
examples are not our devices); `docs/`, `images/`, `stl/`.

## Constraints that shape the review
- **`garage/charger` ESP8266 target is dead** (antenna/WiFi hardware issue). Review the
  ESP32 target only; never compile or "fix" the ESP8266 files.
- **`jkbms_v2` in production has no COM/serial access.** Anything that reboots or panics must
  be diagnosed from Home Assistant/MQTT telemetry; favor changes that add remote evidence.
- **The user compiles and flashes.** Only `esphome config` is run here.
- **Chip revisions differ per unit** (read from boot logs on 2026-10-25): `jbdbms_v2` and
  `jkbms_v2` are ESP32 **rev3.0**; `grid-meter` and `garage/generator` are **rev3.1**.
  `minimum_chip_revision` must never exceed the device's real revision (the OTA fails with
  `Finishing update failed`; a USB flash would not boot). Check a device with
  `esphome logs <yaml> --device <host-or-ip>` (read-only; use `timeout`) and look for
  the `ESP32 Chip:` line, which also shows the running ESPHome version.
- Both BMS gateways keep a BLE client connected permanently next to WiFi + MQTT, so
  BLE/WiFi coexistence changes always matter for them.
- `jbdbms_v2` and `jkbms_v2` set `CONFIG_ESP_TASK_WDT_TIMEOUT_S: "30"` through
  `sdkconfig_options`, so sdkconfig/watchdog changes matter.
- PZEM readings have gone stale before (see the "stale timeout" commits), so modbus/uart
  changes matter for `grid-meter` and `garage/generator`. `garage/generator` declares its
  `modbus` hub explicitly (`garage-generator-pzem.yaml`); `grid-meter` still relies on the
  implicit hub, whose timings are ESPHome's defaults (2000ms response wait, 600ms turnaround).
- The ai-helper PowerShell hooks (`.github/hooks/ai-helper-*.ps1`) call `web_server` REST
  URLs by entity **name** (for example `/button/webhook_reasoning/press`,
  `/number/AI Helper Usage Remaining/set`). `web_server` URL/ID changes need a look at them.

## Related custom code that built-in features may overlap
- `jkbms_v2/panic_capture.h`, `jkbms_v2/tools/decode_panic.py` and the
  `esphome-crash-diagnostics` skill (`panic_capture.h`, `wdt_forensics.h`, `decode_panic.py`):
  panic report for no-serial units. Since ESPHome 2026.3 the crash itself is stored by
  ESPHome's built-in handler (it owns `__wrap_esp_panic_handler`); `panic_capture.h` is only
  an adapter that replays it into text sensors. Never reintroduce a custom wrapper.
- `garage/generator/garage_generator_pzem.h`: `SpikeGuard` filters for the `pzemac` values (the
  guard must pass the stale-zero publish: `publish_state()` runs the filter chain, hence
  `pzem_forcing_zero`). Not a replacement for anything ESPHome ships; `delta`/`throttle` filters
  cannot tell a one-sample glitch from a step.
- Hand-rolled persisted charging-duration counter (`globals` + `interval`) in both BMS
  gateways.
- OLED redraw pattern (`update_interval: never` + gated 1 s `interval:`) in `jkbms_v2` and `jbdbms_v2`.
