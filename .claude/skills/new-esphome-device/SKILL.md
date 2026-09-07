---
name: new-esphome-device
description: Scaffold a new ESPHome device in this repo using the shared <device-name>-common.yaml base package. Use whenever the user wants to create/add a new ESPHome device, board, or node, or asks to "start a new device" here. Ensures every device reuses the common WiFi/AP/captive-portal, OTA, web_server, http_request, time, and diagnostic-sensor baseline instead of duplicating it.
argument-hint: Device name + board + desired capabilities + package split
user-invocable: true
---

# New ESPHome Device

Every device in this repo is composed from packages. The shared baseline lives in a
`<device-name>-common.yaml` file and MUST be included by each device via `packages.common`.
Do not re-declare anything the common package already provides (wifi, ap, captive_portal,
ota, web_server, http_request, time, logger, debug, wifi-forget button, or the WiFi
RSSI / WiFi Channel / WiFi Name / Reset Reason diagnostic sensors).

## Naming Conventions
Follow these for all further development so files stay discoverable and includes stay
consistent (`<device-name>` is kebab-case):

| file                                | role                                          |
|-------------------------------------|-----------------------------------------------|
| `<device-name>-common.yaml`         | **shared base package** (the template)        |
| `<device-name>-<component>.yaml`    | device/component-specific package             |
| `<device-name>-<component>.h`       | C++ include for that component (optional)     |
| `<device-name>-defaults.yaml`       | shared substitution presets (optional)        |
| `<device-name>-device.yaml`         | top-level entry file that composes packages   |

The base package **must** be named `<device-name>-common.yaml`.

## What the common package provides (the template baseline)
- `logger` (INFO), `debug` (reset reason + free heap every 30s)
- `text_sensor`: **Reset Reason** and **WiFi Name (SSID)** diagnostics
- `sensor`: **WiFi RSSI** and **WiFi Channel** (both `update_interval: ${wifi_rssi_update}`;
  channel is read via a `template` sensor lambda —
  `wifi::global_wifi_component->get_wifi_channel()` — since `wifi_info` has no channel
  text sensor)
- `ota` (esphome platform)
- `time` (sntp, id `ha_time`, `timezone: ${time_zone}`)
- `wifi` with fallback `ap` (`${ap_ssid}` / `${ap_password}`), `fast_connect`, `captive_portal`
- `button`: **WiFi Forget** (clears stored credentials, reboots into setup AP)
- `web_server` (port 80) and `http_request` (with watchdog-safe timeouts)

## Required Substitutions
The common package references these substitutions; the device entry file MUST define them:

| substitution        | example              | purpose                          |
|---------------------|----------------------|----------------------------------|
| `device_name`       | `my-device`          | ESPHome node name (kebab-case)   |
| `friendly_name`     | `My Device`          | Display name / entity prefix     |
| `ap_ssid`           | `AP-My-Device`       | Fallback AP SSID                 |
| `ap_password`       | `password`           | Fallback AP password             |
| `time_zone`         | `Europe/Kyiv`        | SNTP timezone                    |
| `wifi_rssi_update`  | `60s`                | RSSI sensor update interval      |

## Steps to Add a New Device
1. Confirm `device_name` (kebab-case) and `friendly_name` with the user if not given.
2. Create the entry file `<device-name>-device.yaml` from the template below.
3. Put device-specific hardware/logic in its own package file
  `<device-name>-<component>.yaml` (+ optional `<device-name>-<component>.h` for C++),
  and add it under `packages:`. Keep only device-specific config there.
4. Reuse shared substitution presets from `<device-name>-defaults.yaml` when relevant.
5. Validate before flashing: `esphome config <device-name>-device.yaml`
  (and `esphome compile ...`). Do not attempt to flash - flashing is a user action.

## Entry-File Template
```yaml
substitutions:
  device_name: <device-name>
  friendly_name: <Friendly Name>
  ap_ssid: AP-<Friendly-Name>
  ap_password: password
  time_zone: Europe/Kyiv
  wifi_rssi_update: 60s

esphome:
  name: ${device_name}
  friendly_name: ${friendly_name}

esp32:
  board: esp32dev
  framework:
    type: esp-idf

packages:
  common: !include <device-name>-common.yaml
  # <component>: !include <device-name>-<component>.yaml

api:
```

Look at the existing `*-device.yaml` entry files for full working examples (including
`platformio_options`, `includes`, and `on_boot` when a `.h` package is used).
