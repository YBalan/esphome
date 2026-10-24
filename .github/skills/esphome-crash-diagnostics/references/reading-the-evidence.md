# Reading the evidence

Contents: reset-reason strings · graph signatures · what `Last Watchdog` tells you ·
known culprits · fixes · when the data is not enough.

## 1. Reset-reason strings (ESPHome `debug` text sensor)
The strings come from `RESET_REASONS[]` in `esphome/components/debug/debug_esp32.cpp`.

| string | meaning | where to look first |
|---|---|---|
| `exception/panic` | abort or CPU fault (null deref, stack overflow, failed allocation, assert) | graphs: memory, stack; then recent code changes |
| `task watchdog` | the main loop did not finish an iteration for the timeout (30 s here) | `Last Watchdog`, `Loop Max Time` |
| `interrupt watchdog` | interrupts were blocked for ~300 ms | flash writes, ISRs, critical sections (section 5) |
| `other watchdogs` | RTC or timer-group watchdog | rare; check for sleep/power code |
| `brownout` | supply dipped below the threshold | power supply, relays, antenna current peaks |
| `Reboot request from X` | ESPHome itself called reboot; X = the component running at that moment | the reboot logic of X (section 4) |
| `software via esp_restart` | restart without a recorded source | OTA, restart button, user action |
| `power-on event` / `external pin` | power cycled or reset pin pulled | not a firmware crash |

`Reboot request from X` is a *symptom* of ESPHome's own safety nets, not a cause. Ask what
made X give up (a stalled loop, lost connectivity) rather than fixing X.

## 2. Graph signatures (24 h history of the Layer 1 sensors)
| what the graph shows | reading |
|---|---|
| `Heap Free` and `Heap Min Free` flat, stack flat | not memory exhaustion: suspect a stalled loop (watchdog) or hardware |
| `Heap Min Free` falls step by step across resets, uptime sawtooth | leak or growth; out-of-memory aborts show as `exception/panic` |
| `Heap Largest Block` collapses while `Heap Free` stays high | fragmentation; large allocations (JSON, strings, TLS, big buffers) fail first |
| `Loop Stack Free` under ~500 B | main-task stack overflow; look for big locals, deep lambdas, `std::string` in loops |
| `Loop Max Time` spikes over 10 s before a reset | a blocking call (network publish, DNS, flash write, BLE call) |
| uptime resets at irregular intervals | event-driven fault, not a fixed timer (a fixed period suggests a counter overflow or scheduled job) |
| resets cluster after a state change (relay, charging, WiFi drop) | correlate with that entity's history |

Read at least two or three resets before concluding; one is an anecdote.

## 3. What `Last Watchdog` tells you
It shows `<component> (uptime Ns)` after a *task* watchdog reset.
- The component is the one the loop **last started**. ESPHome never clears it, so a starved
  (not blocked) loop can show an innocent name. Check whether the named component can block
  at all (network publish, bus read, `delay`) before blaming it.
- `none` means the loop was not inside any component (startup or idle).
- `unknown (no record)` means the hook did not run: the watchdog was not the task watchdog
  after all, or RTC memory was lost (power cycle).
- `n/a` after any other reset reason; that is expected.

## 4. Known culprits seen in this repo
- **MQTT publishing on the main loop.** With `idf_send_async: false` (the ESP32 default)
  each publish runs in the loop and can block up to the 10 s network timeout, twice
  (one immediate retry). A dead or flapping broker produces `Last Watchdog = mqtt`.
  `log_topic` (on by default when `topic_prefix` is set) turns every log line into a publish.
- **`esp32_ble_tracker` "Scan never terminated, rebooting".** Fires when a scan overruns
  `scan_duration * 2` s (default 10 s) after a loop stall, giving `Reboot request from
  esp32_ble_tracker`. It is the same stall as above, caught earlier than the 30 s watchdog.
  Also reboots after 255 failed scan restarts.
- **`reboot_timeout` on `api`, `wifi`, `mqtt`** (default 15 min each): reboots when HA, WiFi
  or the broker is unreachable that long. Shows as `Reboot request from api|wifi|mqtt`.
  Only the symptom of a connectivity problem; set it deliberately.
- **BLE client stacks** (`ble_client`, BMS drivers): `esp_ble_*` calls from the main loop can
  wait on the BLE task. Pin external components to a commit so a rebuild does not change the
  driver silently.

## 5. Interrupt watchdog notes
The task-watchdog hook cannot see it. Typical causes: flash writes (NVS/`preferences`,
OTA) while a high-rate ISR or the radio needs flash-resident code; long critical sections;
a second core holding a spinlock. Mitigations: write preferences less often
(`flash_write_interval`), keep ISRs in IRAM and short, avoid `delay()` in interrupts.
If it recurs, the evidence is circumstantial: correlate with flash-write times and
charging/relay events.

## 6. Fix catalog (verify each key exists in the installed ESPHome first)
Grep `site-packages/esphome/components/<component>/__init__.py` for the option name before
adding it; ESPHome versions differ.
- `mqtt: idf_send_async: true`, `mqtt: log_topic:` (empty) to disable log mirroring.
- Longer or explicit `reboot_timeout` on `api` / `wifi` / `mqtt`.
- `esp32_ble_tracker: scan_parameters:` passive scan, longer interval, if the tracker is
  not needed (see `esphome-patterned-sensors` for stability-first BLE defaults).
- Raising `CONFIG_ESP_TASK_WDT_TIMEOUT_S` hides a stall instead of removing it. Use it only
  as a stopgap and say so.
- Remove or disable a component the device does not actually use (for example an MQTT
  broker connection when only the native API is consumed).

## 7. When the data is not enough
State what is missing and offer the smallest next step: a longer history window, the
`Last Watchdog` value after the next reset, correlation with RSSI/relay/charging entities,
or a one-time serial capture if the user can ever reach the device. Do not name a root
cause the evidence does not support.
