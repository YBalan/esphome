---
name: esphome-crash-diagnostics
description: 'ON-DEMAND ONLY. Add remote crash/restart forensics to an ESPHome ESP32 device that reboots unexpectedly (reset reason "exception/panic", task/interrupt watchdog, or "Reboot request from <component>") and has no serial/COM access, then interpret the evidence. Run only when the user explicitly asks for it, e.g. /esphome-crash-diagnostics. Never apply it while scaffolding a new device, adding sensors, or building the common baseline.'
argument-hint: 'Device YAML path + reset reason(s) seen + how often + whether serial access exists'
user-invocable: true
disable-model-invocation: true
---

# ESPHome Crash Diagnostics (on demand)

Extra, opt-in diagnostics for a device that restarts on its own. It sits **on top of** the
baseline every device already gets from `<device-name>-common.yaml` (`debug`, Reset Reason,
WiFi RSSI/Channel/Name). It is deliberately not part of that baseline: it adds sensors, an
RTC-memory hook and C++ to the firmware, which a healthy device does not need.

## When to use / when not
- Use when the user asks for it by name or asks to investigate periodic restarts of a
  specific device.
- Do **not** use while creating a device, adding ordinary sensors, or editing the common
  package. If the request is "new device", follow `new-esphome-device` and leave this out.
- Do not add it to `<device-name>-common.yaml`. It goes in the one device's own YAML (or a
  `<device-name>-crashdiag.yaml` package) so other devices stay untouched.

## Assumptions (and why)
- Production devices usually have **no COM port**. Panic text and backtraces go to UART0;
  since ESPHome 2026.3 a built-in crash handler also keeps a short crash record across the
  reboot and logs it to the first API client that connects (and then clears it). ESPHome
  still does not ship `espcoredump`, and nothing there lands in Home Assistant history, so
  serial logs are not the plan: the evidence has to come from HA history. Do not ask for
  serial logs unless the user says they have access.
- This skill needs **ESPHome 2026.3 or newer** for Layer 2a. Check `esphome version` first;
  on an older release the built-in handler does not exist and `assets/panic_capture.h`
  would record nothing.
- The user compiles and flashes. Validate with `esphome config` only. Never start
  `esphome compile`/`run` in a device's build directory while the user may be building it:
  two builds in the same `.esphome/build/<name>` delete each other's objects and the link
  fails.
- `esphome config` prints secrets. Redirect it to a file in the scratchpad and grep for the
  few lines you need.

## Procedure
1. **Gather**: device YAML path, the reset-reason strings seen (HA Logbook or the Reset
   Reason entity history), roughly how often, ESP32 + `esp-idf` or not. The hooks below
   need an Xtensa ESP32 (original ESP32, S2, S3) with esp-idf; the telemetry works on any ESP32.
2. **Read the YAML first.** `debug:` and each `platform: debug` sensor/text_sensor can only
   appear once per device, so extend the existing entries (the common package already has
   `debug:` and the Reset Reason text sensor) instead of pasting duplicates.
3. **Layer 1, telemetry** (always): add the blocks in `assets/diagnostics.yaml`: heap free /
   largest block / min free / fragmentation, loop max time, uptime, loopTask stack free.
   These separate the three families of cause (memory exhaustion, stack overflow, stalled
   main loop) from the graph alone.
4. **Layer 2a, panic report** (for `exception/panic` and interrupt-watchdog resets; the
   strongest tool here): copy `assets/panic_capture.h` next to the device YAML and add it to
   `esphome: includes:`, add the `logger: on_message` hook, the two `Last Panic` text
   sensors, the `on_boot` lambda and (optionally) the two disabled-by-default test buttons
   from `assets/diagnostics.yaml`. ESPHome's own crash handler stores the exception, crashed
   core, PC, fault address and both cores' backtraces across the reboot; `panic_capture.h`
   replays that at boot through the logger and publishes it, so it also sits in HA history.
   **Never** add `-Wl,--wrap=esp_panic_handler` or define `__wrap_esp_panic_handler`:
   ESPHome already does, and the link fails with "multiple definition". The task name and
   uptime of the old wrapper are not available. Copy `assets/decode_panic.py` as the
   decoder (`esphome logs` also decodes the `BT` lines itself when it is connected at
   boot). Needs the esp-idf framework; the handler is ESPHome's, so other chips follow
   ESPHome's support, but only an original ESP32 on 2026.10.0 was built and checked.
5. **Layer 2b, task-watchdog component** (cheap, optional with 2a): copy
   `assets/wdt_forensics.h`, include it, add the `Last Watchdog` sensor and `on_boot` lambda.
   It names the ESPHome component the loop was in when the *task* watchdog fired.
6. **Layer 3, chart**: give the user the `history-graph` card from `assets/diagnostics.yaml`.
   Entity IDs are generated by HA; tell the user to pick them with the entity picker or read
   them from Developer Tools → States. Do not guess them.
7. **Verify, then hand over.** Run `esphome config` (output to file). For C++ changes also
   compile once, but in a *copy* of the project under the scratchpad (it gets its own
   `.esphome/build`), never in the user's folder, and delete the copy afterwards because it
   holds `secrets.yaml`. (If the user asks you to build in their folder, first check no
   other build is running there.) The link must succeed with a single
   `__wrap_esp_panic_handler`: a "multiple definition" error means a custom wrapper is
   still present. Say plainly what was
   verified (config, compile, decode of a known address) and what was not (a real panic on
   the device). Flashing is the user's.
8. **Tell the user to keep the ELF.** `firmware.elf` sits in
   `.esphome/build/<name>/build/` (native ESP-IDF builds, ESPHome 2026.7+; older PlatformIO
   builds used `.pioenvs/<name>/`) and a rebuild overwrites it, so each flashed
   build's ELF must be copied aside (with a date) or its panics cannot be decoded.
9. **Read the evidence** after the next reset(s) with `references/reading-the-evidence.md`:
   it maps reset reason + graph shape + `Last Panic` / `Last Watchdog` to likely causes and
   fixes. One reset is a hint, two or three with the same signature is a pattern.

## Limits to state to the user
- Panic capture only helps for builds that contain it. Panics from the firmware already on
  the device, and any older build whose ELF was lost, cannot be decoded.
- It records the call stack, not the cause. A backtrace ending in `abort` + a function name
  says *where*; why still needs reading the code.
- `Last Watchdog` names the component the loop *last started*. ESPHome never clears it, so
  if the loop was starved rather than blocked the name can be innocent. Treat it as the
  prime suspect, not proof.
- Reset-reason and uptime history show *that* and *when*, not *why*. Say so when the data
  is thin instead of naming a cause with false confidence.

## Output
Summarise in this order: what was added (per layer), what the user must do (flash, add the
card), what to look at after the next reset, and the limits that apply to this device.
