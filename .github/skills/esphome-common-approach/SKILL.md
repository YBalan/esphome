---
name: esphome-common-approach
description: 'Common ESPHome troubleshooting and implementation workflow for flaky sensors, RF capture issues, API disconnects, and deployment problems. Use when debugging behavior regressions, making safe YAML/C++ helper changes, validating config, deploying via OTA/serial, and confirming runtime stability with logs and counters.'
argument-hint: 'Describe symptom + target device (example: RF learns wrong code on generator)'
user-invocable: true
---

# ESPHome Common Approach

## When to Use
- Sensor values become unknown/stale or do not recover.
- RF learn/transmit works intermittently, captures wrong code, or stops capturing.
- Home Assistant API shows backlog/disconnect symptoms.
- A change compiles but behavior regresses on device.
- OTA or serial deployment fails and needs structured fallback.

## Inputs
- Device YAML path.
- Optional helper header path used by `includes:`.
- Current symptom, expected behavior, and latest logs.
- Deployment target (`COMx` or `device.local`).

## Procedure
1. Baseline and isolate.
- Reproduce once and capture the shortest useful log window.
- Identify if the failure is compile-time, transport/deploy, or runtime behavior.
- Verify whether the running firmware is actually the expected build hash.

2. Read only the relevant sections.
- Inspect the smallest YAML blocks tied to the symptom (component, automations, intervals, scripts).
- Inspect helper functions used by those lambdas.
- Avoid broad refactors until behavior is stable.

3. Pick the least risky fix first.
- Prefer rollback to the last known-good behavior before adding new logic.
- Apply one focused change set per hypothesis.
- Keep hardware safety defaults conservative (for example, disable physical GPIO actions when not wired).

4. Preserve behavior boundaries.
- Keep expensive logic in helper C++ and keep YAML glue thin.
- For runtime counters, gate accumulation by explicit run-state.
- For stale telemetry, use freshness checks and controlled zeroing.

5. Validate before deploy.
- Run `esphome config <yaml>` after each edit.
- If invalid, fix schema/type issues before any upload attempts.

6. Deploy with fallback strategy.
- Prefer OTA for speed when stable.
- If OTA fails with resets/timeouts, use serial upload.
- If serial port is busy/broken, stop log holders, retry with `--no-logs`, then retry upload.

7. Verify on-device behavior.
- Confirm target symptom is fixed.
- Confirm no new warnings dominate logs (`component took a long time`, API buffer pressure, repeated disconnects).
- Recheck related features that can regress (learn mode, run-duration sensors, energy counters, HA entity updates).

8. Finish with clear state.
- Summarize what changed and why.
- Provide exact test steps for the user to confirm.
- Note unresolved risks and the next smallest follow-up.

## Decision Points
- If symptom appears after a recent change: revert that specific change first.
- If capture happens but value is wrong: tune decode parameters and avoid over-aggressive filtering/gating.
- If nothing captures: remove extra gating and restore direct known-good capture path.
- If API disconnects but WiFi is stable: reduce component pressure before changing network assumptions.
- If OTA upload repeatedly fails mid-transfer: switch to serial recovery path.

## Quality Checks
- Config valid with zero errors.
- Firmware deployed to the intended device (not just compiled).
- Behavior verified in runtime logs and HA entities.
- No unrelated code paths changed.
- Safety defaults remain intentional.

## Completion Criteria
- Primary symptom is reproducibly fixed.
- At least one regression-prone adjacent behavior is re-verified.
- User has explicit next test steps and fallback path.

## Example Prompts
- `/esphome-common-approach RF learn captures wrong binary on generator.`
- `/esphome-common-approach PZEM values go unknown after a few minutes.`
- `/esphome-common-approach OTA keeps disconnecting at 20-80 percent.`
