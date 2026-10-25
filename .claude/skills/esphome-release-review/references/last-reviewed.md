# Last reviewed

The review step in `SKILL.md` starts here and ends by rewriting this file.

- **Last reviewed ESPHome version:** 2026.10.0 (range reviewed: 2026.3.0 - 2026.10.0, coming from 2026.2.4)
- **Reviewed on:** 2026-10-25
- **Installed ESPHome at review time:** 2026.10.0 (upgraded with `python -m pip install --upgrade esphome`)
- **Source quality:** WebFetch summaries of `https://esphome.io/changelog/<ver>/`. PR numbers
  below are as reported there and were **not** checked against the PRs. Not read: the part
  of the 2026.10.0 page after 200k characters and the part of the 2026.6.0 page after 100k.
- **Applied from this review:** `minimum_chip_revision` "3.1" -> "3.0" in `jbdbms_v2` and
  `jkbms_v2` (see "Confirmed on real devices"); the `garage/generator` PZEM rework (see
  "PZEM rework"). The OLED gated redraw in `jbdbms_v2` was unrelated to ESPHome 2026.10.

## Validation baseline (`esphome config` on 2026.10.0)

Passing: `jbdbms_v2/jbdbms_v2.yaml` (with the OLED change), `jkbms_v2/jkbms_v2.yaml`,
`grid-meter/grid-meter-device.yaml`, `electrolux-boiler/electrolux-boiler.yaml`,
`electrolux-boiler-lcd/electrolux-boiler-LCD.yaml`, `wc-lights/wc-lights.yaml`,
`ai-helper/ai-helper.yaml`, `ai-helper/semaphore-device.yaml`,
`garage/charger/garage-charger-esp32-device.yaml`, `garage/generator/garage-generator.yaml`
(also compiles after the `servo.write`/`mqtt.publish` fix, see "Fixed"),
`RS232/logicpower/logicpower-{pip8048,powermanager,protocol-test,ups-device}.yaml`.

Already failing before the upgrade, not caused by it (not confirmed on 2026.2.4):
- `electrolux-boiler-remote/electrolux-boiler-remote.yaml`: invalid YAML at line 30
  (`red: 100%, green: 100%, blue: 100%` on one line under `color:`).
- `ai-helper/mouse-mover-device.yaml`: `Couldn't find ID 'sem_usage_remaining'`; the LCD
  lambda in `ai-helper-mouse-mover.yaml` uses `id(sem_usage_remaining)` inside
  `#ifdef AI_HELPER_SEMAPHORE_BUILD_ENABLED`, but ESPHome resolves the ID regardless of the `#ifdef`.

Not checked: `electrolux-boiler-lcd/test.yaml`, the dead ESP8266 charger target, vendored
`esphome-jbd-bms/` examples. **Nothing was compiled**; lambdas, `.h` files and external-component
C++ (`syssi/esphome-jbd-bms@main`, `syssi/esphome-jk-bms@main`) are untested against 2026.10.

Note: on 2026.2.4 `esphome config` failed for `jbdbms_v2` because `syssi/esphome-jbd-bms@main`
uses `cv.sensitive`, which that ESPHome lacked. Upgrading fixed it.

## Digest 2026.3 - 2026.10 (what mattered for this repo)

### Opportunities (new features that overlap with our custom work)
- **Built-in crash handler** (2026.3 #14709): backtraces captured and reported across
  reboots; both cores (#15559); kept across OTA rollback (#15578); fault address and raw
  cause (2026.8); flags records from a different firmware build (#17770); per-core addr2line
  hint (#18418); garbled-trace fix (#17939). Overlaps `panic_capture.h`, `decode_panic.py` and
  the `esphome-crash-diagnostics` skill (which assumes ESPHome ships nothing like this).
  Open question: how it surfaces without the native API (our devices use MQTT).
- **ESPHome logs over `web_server` (HTTP SSE)** (2026.8 #17110): would give `jkbms_v2`
  (no serial in prod) live logs. Its `web_server:` block is currently commented out.
- **Watchdog**: `watchdog_timeout` option (2026.5 #15908); default panic on timeout
  (#16142). May replace `CONFIG_ESP_TASK_WDT_TIMEOUT_S: "30"` in `sdkconfig_options`.
  Exact YAML key not checked. sdkconfig ints are no longer silently clamped (#16515).
- **OTA / safe mode**: OTA rollback in core, `safe_mode.mark_successful` (2026.3 #14306),
  soft-brick recovery via recovery partition (#16339), partition-table and bootloader OTA
  (#15780, #16238), retry on network errors (#18332), downgrade protection (#17315),
  Noise-encrypted OTA (2026.9 #18489) with `allow_plaintext_upload` (2026.10 #19396),
  compressed uploads (#19037).
- 2026.10: `loop_interval` / `suspend_loop`, persistent `counter` sensor, exponential moving
  average preserved across reboots, `uart` `apply_settings_live()`, `time` template platform.
  The persistent counter could replace our hand-rolled charging-duration `globals`.

### Verify on device after flashing
- **BLE/WiFi coexistence** (both BMS gateways): `COEX_PREFER_BT` held for the life of any BLE
  connection (2026.5 #16036, breaking); reverted to balanced during OTA (#19082); warning when
  scan window > 600 ms with WiFi (#18725); missed adverts with WiFi fixed (#18356); stale
  Bluedroid request guards (#19375, #19859); `ble_client` state-corruption crash fix (#14919).
  Watch for WiFi/MQTT drops while BLE is connected.
- **modbus rework** (`grid-meter`, `garage/generator`; `pzemac`): microsecond bus timing
  (#12421), per-frame queue state machine (#17922), CRC for unknown function codes (#18483),
  `pzemac` typed read callback (#18855). `modbus_controller` breaks are not relevant (unused).
  Watch for stale/unknown PZEM values.
- **UART**: wake-on-RX default on ESP32 (2026.3 #14391).
- **CPU**: default frequency is now the chip maximum (2026.4 #15143), so ESP32 runs at 240 MHz.
- **time**: libc timezone code removed (#13635), timezone fixes (#16583). Check local time.
- **Scheduler/loop**: main loop cadence decoupled from scheduler wake timing (2026.5 #15792).
- **preferences**: `IntervalSyncer` became a `PollingComponent` (2026.9); relevant to
  `flash_write_interval: 30s`.
- **web_server**: object-ID URL matching removed, v1 deprecated (2026.7); JSON entity ID
  format changed, `name_id` dropped (2026.8). The ai-helper hooks use entity names, so
  expected to be fine; smoke-test one call.

### Removed / deprecated APIs (grep result: none used in our own files)
`wifi_ssid()`, `set_retry` / `cancel_retry`, `ESPBTUUID::to_string()`, `IPAddress::str()`,
sensor `raw_state`, modbus `send()` / `disable_crc` stub / `waiting_for_response()`,
`custom_components` folder, `cv.only_with_esp_idf`, `LogListener`, i2c `readv`/`writev`.
Remaining risk is inside external components, which only a compile will show.

### No relevant entries found
`ssd1306_i2c`, `factory_reset` button. `lcd_pcf8574`: only a `lcd_base` `millis()` truncation
fix (2026.3) and IDF 6 I2C NACK handling (2026.4).

## Confirmed on real devices (2026-10-25)
- **Chip revisions** (boot-log line `ESP32 Chip: ESP32 rev…`, read with
  `esphome logs <yaml> --device <host>`): `jbdbms_v2` rev3.0, `jkbms_v2` rev3.0,
  `grid-meter` rev3.1, `garage/generator` rev3.1. Others not read.
- **OTA refused by the device**: building `jbdbms_v2` with 2026.10.0 produced an image with
  minimum chip rev v3.1 (`minimum_chip_revision: "3.1"` -> `CONFIG_ESP32_REV_MIN_FULL=301`).
  The rev3.0 chip rejected it in `esp_ota_end()`; ESPHome showed `Finishing update failed`
  (response 0x84, `ESP_ERR_OTA_VALIDATE_FAILED`). The running 2026.2.4 builds, made with the
  same "3.1", evidently did not enforce it on this chip. Fix: "3.0" in `jbdbms_v2` and
  `jkbms_v2`; both must be rebuilt (the `jkbms_v2` build made earlier that day still had 3.1).
  Never set the minimum above the lowest chip revision of the device: a USB flash of such an
  image would not boot at all (ESPHome docs: "will not boot on chips older than…").
- **`grid-meter` flashed fine on 2026.10.0** (rev3.1 chip, "3.1" is right). New boot warnings:
  `flash_chip: generic` (drops unused flash-vendor drivers, ~1 KB IRAM, harmless in DIO mode)
  and `sram1_as_iram: true` (+40 KB IRAM, original ESP32 + esp-idf only; needs a bootloader
  from ESP-IDF >= 5.1; USB flashing updates the bootloader, OTA does not; an old bootloader
  will not boot it). The device prints that suggestion only when its bootloader supports it.
  Not applied to `grid-meter`. The BMS gateways are OTA-only, so their bootloader age was
  unknown; `jbdbms_v2` printed the same hints after its OTA (flash vendor `gd`), proving its
  bootloader supports it, and got `flash_chip: gd` + `sram1_as_iram: true` (config valid, not
  yet rebuilt). Apply them to another unit only if that unit prints the hint itself, and use
  the vendor it names (it can differ per unit: `grid-meter` said `generic`, `jbdbms_v2` `gd`).
- **The chip-revision fix worked**: `jbdbms_v2` took the OTA and now runs 2026.10.0.
- **`jkbms_v2` first build failed at link** (compiled in place at the user's request): 
  `multiple definition of __wrap_esp_panic_handler` between `panic_capture.h` and ESPHome's
  `esp32/crash_handler.cpp`. Since 2026.3 the esp32 component always adds
  `-Wl,--wrap=esp_panic_handler` and installs its own handler (no option to turn it off), so
  a custom wrapper can no longer coexist. Fix applied: `panic_capture.h` is now an adapter
  that replays the built-in handler's `esp32.crash` ERROR lines (`logger: on_message` +
  `crash_handler_log()` at boot) into the same "BMS Last Panic" sensors; the redundant
  `platformio_options: build_flags` was removed. Second build OK (ESPHome 2026.10.0, IDF
  5.5.6, no warnings besides GPIO strapping pins; flash 77%, IRAM 82% used; image min chip rev
  v3.0). Not yet tested with a real panic (use the two disabled-by-default test buttons).
  Task name and uptime are no longer recorded in the panic report.
- **Native ESP-IDF builds are the default now**: output is in
  `.esphome/build/<name>/build/` (`build/firmware.elf`), not `.pioenvs/<name>/`.
- **`esphome: platformio_options: build_flags` is deprecated** and removed in **2026.12.0**.
  It is still honored (routed to `build_flags`) with a warning. Used by `jkbms_v2`
  (`-Wl,--wrap=esp_panic_handler` for `panic_capture.h`), the three `ai-helper` device
  files, and the `esphome-crash-diagnostics` assets. Migrate to `esphome: build_flags:`.

## PZEM rework (`garage/generator`, 2026-10-25)

Triggered by "rework work with pzem according to the ESPHome release changes". No newer release
than the reviewed one, so the earlier modbus/`pzemac` entries were **confirmed from the installed
2026.10.0 source** (`components/modbus`, `components/pzemac`), not from the PRs.

Confirmed in the source:
- `pzemac` is a `modbus::ModbusClientDevice` and publishes from `on_read_input_registers`; a CRC or
  length failure never reaches it (the hub drops the frame and logs a `modbus` WARN), so a spike is a
  frame that is valid on the wire but wrong in value.
- `modbus_id` is a `GenerateID`. Without a `modbus:` block `esphome config` prints `modbus: []` yet
  passes, and `grid-meter` flashed fine that way. The hub's Python defaults are
  `send_wait_time: 2000ms` (max wait for a reply; before 2026.x it was a 250ms gap between sends)
  and `turnaround_time: 600ms` (quiet time before the next request). The generator's commented-out
  `send_wait_time: 200ms` is the old meaning, i.e. today's `turnaround_time`.
- `Sensor::publish_state()` runs the whole filter chain, so any hand-published value (our stale
  zeroing) goes through the same filters as a PZEM reply.
- Filter lambdas are `optional<float>(float)` and `esphome::optional` is `std::optional`; returning
  `std::nullopt` drops the sample.

Applied (config validates; entity ids and names unchanged except one added diagnostic):
- New `garage-generator-pzem.yaml` (uart, explicit `modbus_pzem` hub with `send_wait_time: 500ms`,
  `turnaround_time: 200ms`, `pzemac` with `modbus_id`/`address`, stale-zero `interval`) and
  `garage_generator_pzem.h` (moved out of `garage_generator_logic.h`, which now includes it).
  `gpio_uart_rx/tx` stay in the main substitutions. The `pzem_last_rx_ms` YAML global became an
  inline variable in the header.
- `SpikeGuard` as the first filter of every value: outside the hard limits -> dropped; within
  `max_step` of the last good value -> accepted; otherwise held back until it repeats (2 samples) or
  persists (4 samples). Energy is monotonic. Limits (V 0-300/40, A 0-100/20, W 0-23000/3000,
  Wh 0-9999990/50, Hz 40-70/5, PF 0-1/0.5) are **my estimates for a 100 A PZEM-004T v3 polled every
  2 s, not measured**; tune them from the `PZEM Rejected Samples` counter and the `pzem_guard` log.
- The freshness clock is now refreshed by the guard (any real reply) instead of the voltage
  `on_value`, which ran after the 2 s throttle. Stale zeroing sets `pzem_forcing_zero` so its zeros
  bypass the guards, then resets them so the first real reply is a new baseline.
- Host test of the real header (stubbed ESPHome headers, `ziglang` clang from a scratchpad venv): 9
  scenarios, all pass (steady, 0 V dip, out-of-range, 230 -> 180 V step, ramp, energy glitch up/down,
  energy reset, stale zero and recovery, boot).
- Compile evidence: after the fix below, `esphome compile garage-generator.yaml` (compiled in place
  at the user's request, ESPHome 2026.10.0, IDF 5.5.6) links: RAM 39.0%, flash 58.5%, image min chip
  rev v3.0 (safe for rev3.0 and rev3.1 units). The only warning is old code (`snprintf` truncation at
  `garage_generator_logic.h` `"AP:%s"`); nothing from `garage_generator_pzem.h` or the new YAML
  lambdas warns. **Not flashed**, so nothing here was seen running.

### Fixed: `garage/generator` did not compile on 2026.10 (found by the first compile)
Reproduced on the **unmodified HEAD** files (`esphome compile --only-generate` on a copy), so it was
not caused by the PZEM rework. `esphome config` passes, which is why the earlier review missed it.
Fixed by turning the 28 `servo.write` actions into `lambda:` actions calling `id(servo_N).write(...)`
(adjacent servo_1/servo_2 writes merged into one lambda) and the `mqtt.publish` payload into a
`lambda:` calling `mqtt::global_mqtt_client->publish(...)`. Original analysis:
- Cause: in 2026.10, templated *apply-actions* (`servo.write`, `mqtt.publish`, `pzemac.reset_energy`
  style actions) are emitted as global `static void esphome__applyaction_id_N__fn()` functions
  **above** the globals, and `esphome: includes:` headers are added at the very end
  (`add_includes`, `CoroPriority.FINAL`). A `!lambda` value inside such an action that calls a
  helper from an included header gets `'garage' has not been declared`. No include position can fix
  it, because the header itself needs globals declared after those functions.
- Sites: 28 `servo.write: level: !lambda "return garage::angle_to_servo_level(...)"` in
  `garage-generator-servos.yaml` and 1 `mqtt.publish` payload
  (`garage::current_run_duration_text`) in `garage-generator-mqtt.yaml`. Compiler messages name the
  wrong yaml file/lines because of the package merge.
- Plain `lambda:` actions, sensor/filter lambdas and `interval` bodies are emitted inside `setup()`
  (after the include) and are fine; that is why `grid-meter`, `jkbms_v2` and `jbdbms_v2` built.
- Rule of thumb for this repo: inside actions that ESPHome turns into typed actions (`servo.write`,
  `mqtt.publish`, other `register_apply_action` actions), do not call `includes:` helpers from a
  `!lambda` value; use a plain `lambda:` action instead.
- Not checked: whether `ai-helper`, `electrolux-*`, `wc-lights` or `RS232/logicpower` have the same
  pattern. Check with `esphome compile --only-generate` on a scratchpad copy and look for lines that
  use a header namespace before the `#include` line in `main.cpp`.

Not done / findings left open:
- `grid-meter` untouched. Its range lambdas return `0.0` for out-of-range samples, which is itself a
  spike (to 0). Its stale-zero `interval` is commented out and `mark_pzem_rx_if_finite` is not wired,
  so its `grid_meter::` freshness helpers are currently dead code. It still uses the implicit hub.
- `Energy` is zeroed on stale (commit 525e294, kept on purpose). With `state_class: total_increasing`
  Home Assistant treats 0 as a reset and the return to the real counter as new consumption, so each
  generator stop/start can add the whole PZEM counter to the energy statistics. Decide whether to
  stop zeroing energy (leave the last value, or publish unknown).
- Freshness limit: `kPzemMaxAgeMs` is 30000 (the user raised it from 5000 after the rework; the
  host test used 5 s but only to prove the logic). It now takes 30 s of silence before the values
  read 0 and `Generator Running` turns off.

## Open items carried forward
1. Done for `jkbms_v2` (adapter over the built-in handler, builds). Still to verify on the
   device: press a test-panic button after flashing and check both "BMS Last Panic" sensors
   fill in. The `esphome-crash-diagnostics` skill assets were updated to the same design.
2. Decide: enable `web_server` log streaming on `jkbms_v2`.
3. Decide: replace the sdkconfig watchdog setting with `watchdog_timeout`.
4. User to rebuild and flash `jbdbms_v2` and `jkbms_v2` (min chip rev fix), then watch
   coexistence (BMS gateways) and PZEM stability.
5. Migrate `platformio_options: build_flags` to `esphome: build_flags:` before 2026.12 in the
   three `ai-helper` device files (`jkbms_v2` no longer uses it; the crash-diagnostics
   skill was updated, including the native `build/firmware.elf` path).
6. `jkbms_v2` uses 82% of IRAM: once its own boot log prints the `flash_chip` /
   `sram1_as_iram` hints after the next OTA, apply what it names (see above).
7. `grid-meter` (`flash_chip: generic`) and `jbdbms_v2` (`flash_chip: gd`) now carry
   `flash_chip` + `sram1_as_iram: true` in YAML; `jbdbms_v2` needs a rebuild + OTA to take
   effect, and `grid-meter` a rebuild to drop its last warning.
8. Fix the two pre-existing broken configs listed above.
9. If a new ESPHome is installed, run this review again from 2026.10.0.
10. `garage/generator` now compiles on 2026.10 (see "Fixed"): user to flash it (OTA is safe, image
    min chip rev v3.0) and check the servos still move (the 28 rewritten writes are untested on the
    device), the `current_run_duration` MQTT topic still publishes, and the PZEM values behave.
11. Generator PZEM: after flashing, watch `Generator PZEM Rejected Samples` and the `pzem_guard` /
    `modbus` WARN lines, check `Generator Running` does not flap, and tune the guard limits in
    `garage_generator_pzem.h` if valid readings are being dropped.
12. Decide whether energy should still be zeroed when the PZEM is stale (Home Assistant
    `total_increasing` side effect, see "PZEM rework").
13. `grid-meter` PZEM: same hub pinning and spike guards if wanted; its `0.0` range lambdas and its
    commented-out stale interval are the open points.
