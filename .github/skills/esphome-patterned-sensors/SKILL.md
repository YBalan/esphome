---
name: esphome-patterned-sensors
description: 'Generate routine ESPHome sensor blocks for existing or new devices using project conventions. Use for patterned sensors, substitutions/globals wiring, persistence decisions, and safe validation commands.'
argument-hint: 'Device YAML + sensor pattern + transport + update/filter needs + persistence rules'
user-invocable: true
---

# ESPHome Patterned Sensors

## When to Use
- You want to add common, repeatable sensor patterns to an ESPHome device.
- You are creating a new device YAML and want it aligned with repository conventions.
- You need a safe, minimal sensor block plus required substitutions, globals, and checks.

## Inputs
- Target YAML path.
- Device type and transport: UART, BLE, RF, I2C, ADC, or template-only.
- Sensor set to add: power, energy, runtime, uptime, temperature trend, status, availability, RSSI.
- Update cadence and filtering needs.
- Persistence policy: what must survive reboot and what must be ephemeral.
- Optional C++ helper include path when sensor logic needs lambdas or protocol parsing.

## Hard Rules
- Keep credentials out of YAML and use secrets references.
- Prefer substitutions for device identity, pin values, timing constants, and labels.
- Keep YAML labels and C++ constants/maps synchronized when both exist.
- Use persisted globals only for long-lived operational state.
- Keep logger defaults conservative unless debugging is requested.
- For BLE plus WiFi devices, use stability-first defaults unless the user requests tuning.
- Put all possible runtime configuration in Home Assistant entities (number/select/switch/text) instead of hardcoded YAML constants.
- Keep compile-time-only values limited to hardware and protocol essentials (pins, board, secrets, protocol constants).

## Immutable Baseline (Do Not Change During Sensor Work)
Unless the user explicitly asks to change them, treat these as fixed:
- `esp32.framework.type: esp-idf`
- `ota` present with `platform: esphome`
- `wifi` retains AP fallback and `fast_connect: true`
- `captive_portal` present
- `api` present for HA-integrated devices
- `mqtt` only when requested, with `discovery: true` and stable `topic_prefix`

## Procedure
1. Inspect target context.
- Read only the YAML sections related to the requested sensors.
- If includes are used, inspect only the helper code that those sensors call.

2. Choose pattern set.
- Pick the minimum blocks needed: substitutions, globals, sensor entries, and automations.
- Reuse existing naming style from the target file.

3. Generate patch set.
- Add or update substitutions for timing, thresholds, GPIO, and labels.
- Add globals with explicit persistence choice and safe initial values.
- Add sensor blocks with update interval and filters matched to requested cadence.
- Expose tunable parameters to Home Assistant entities whenever feasible.
- Add lambda guards for state safety when helper logic is used.

4. Return complete output bundle.
- YAML snippets grouped by section:
  - substitutions
  - globals
  - sensor or text_sensor or binary_sensor blocks
  - optional script or interval hooks
- Any matching C++ helper changes required by the YAML.
- Validation commands for the exact target YAML.

5. Validate if code is changed.
- Run config validation on the target YAML.
- Run compile validation when YAML or helper headers were touched.

## Output Contract
Always provide results in this order:
1. Assumptions
2. Added or Updated Substitutions
3. Added or Updated Globals
4. HA Configuration Entities (number/select/switch/text)
5. Sensor Blocks
6. Optional Helper Code Changes
7. Validation Commands
8. Risks and Follow-up Checks

## Pattern Library Guidance
- Runtime counters:
  - Persist totals and offsets.
  - Gate accumulation by explicit running state.
- Energy sensors:
  - Ensure non-resetting semantics are clear.
  - Keep interval and power-source assumptions explicit.
- RF-related status sensors:
  - Persist learned values.
  - Keep temporary capture text non-persistent.
- Trend sensors:
  - Require minimum samples before publishing directional state.
  - Add finite-value checks before math.
- BLE telemetry:
  - Prefer passive scan when reliability is prioritized.
  - Avoid aggressive update rates that can starve API traffic.
- RSSI diagnostics (if available):
  - Add `wifi_signal` sensor for WiFi devices.
  - Optionally add a percent representation using `copy` when useful for dashboards.
  - Mark RSSI entities as diagnostics-oriented and keep update cadence moderate.

## Safety Checks Before Completion
- No hardcoded credentials introduced.
- No direct packet indexing without size validation.
- No checksum-dependent state writes before checksum validation.
- Persistence choices are intentional and documented in assumptions.
- Runtime-tunable values are exposed via Home Assistant entities where feasible.
- Immutable baseline sections remain unchanged unless explicitly requested.
- Validation commands are included and target the real YAML path.

## Example Prompts
- /esphome-patterned-sensors Add runtime and energy counters to garage generator with 30s cadence and persistent totals.
- /esphome-patterned-sensors For a new BLE battery device, add stable telemetry sensors with conservative filters.
- /esphome-patterned-sensors Add temperature trend and availability sensors using helper lambdas and safe guards.
- /esphome-patterned-sensors Add RSSI diagnostics if available and move update thresholds to Home Assistant controls.