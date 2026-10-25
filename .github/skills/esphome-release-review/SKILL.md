---
name: esphome-release-review
description: 'Review ESPHome release notes against this repo and rework the affected devices. Use when the user says "according to the last ESPHome changes", "what is new in ESPHome", "check the new ESPHome release", "rework for the new ESPHome", or right after upgrading ESPHome. Finds releases newer than the last reviewed one, maps changes to the devices and components in this repo, flags breaking changes and removed APIs, then reports, proposes or applies reworks.'
argument-hint: 'Optional: device/component to focus on, or a version range (example: jkbms_v2, or 2026.10 -> 2026.12)'
user-invocable: true
---

# ESPHome Release Review

Turns "according to the last ESPHome changes" into a repeatable pass: read only the
releases that are new since the last review, keep only what touches this repo, and either
report it or rework the YAML/C++. State lives in `references/`, so each run starts where
the previous one stopped.

## When to use / when not
- Use when the user refers to recent ESPHome changes, release notes, an upgrade, or asks
  whether devices should be reworked for a newer ESPHome.
- Not for debugging one misbehaving device (use `esphome-common-approach`) or adding a
  device (use `new-esphome-device`). A finding here may hand over to those.

## Inputs
- Optional focus: a device folder, a component, or a version range.
- Baseline: `references/last-reviewed.md` (last reviewed version, validation state, open items).
- Device and component map: `references/project-map.md`.

## Procedure
1. **Baseline.** Run `esphome version`. Read `references/last-reviewed.md` for the last
   reviewed version. Find the newest release with
   `python -m pip index versions esphome` (first line). Review range = last reviewed
   (exclusive) to the **installed** version. If a newer release exists than the installed
   one, say so; upgrading the user's ESPHome is their call, so ask unless they already
   asked for it. If last reviewed == installed, say there is nothing new and stop (offer
   to re-check open items instead).
2. **Read the changelog pages**, one per minor release in the range:
   `https://esphome.io/changelog/<YYYY.M.0>/` (for example `2026.10.0`). Patch releases
   are not separate pages; check the end of the minor page or the GitHub releases.
   - Each page is 100-250k characters and WebFetch only covers about the first 100k per
     call. Re-call with `offset: 100000`, `200000`, ... until the result says nothing is
     left. Releases are independent, so fetch them in parallel.
   - Give WebFetch a focused prompt: "only entries touching <components from the project
     map>; flag breaking changes, removals and deprecations; keep PR numbers".
   - The answer is a summary by a small model. Treat it as a pointer. Before changing code
     for an entry, confirm it from the PR (`gh pr view <n> --repo esphome/esphome` if `gh`
     exists) or the component docs page.
3. **Map to this repo.** Keep only entries for components listed in the project map, plus
   anything global (core, esp32/ESP-IDF, scheduler, logger, wifi, ota, time, preferences,
   `packages`/`substitutions`/`!include`, sdkconfig, `external_components`).
4. **Grep for removed or renamed APIs.** From the "removed / deprecated / renamed" entries
   build a pattern and search `*.yaml`, `*.h`, `*.cpp`, `*.py` in the repo. Exclude
   `**/.esphome/**`, `esphome-jbd-bms/` (vendored copy) and build folders. Example from
   the 2026.3-2026.10 review: `wifi_ssid()`, `set_retry`, `cancel_retry`,
   `ESPBTUUID::to_string`, `IPAddress::str`, `raw_state`, `custom_components`.
5. **Check external components.** For each source in the project map, see whether it is
   pinned or `@main` and whether upstream now requires a newer ESPHome (look at upstream
   recent commits and the cached copy in `<device>/.esphome/external_components/<hash>`;
   read it, never edit it). `@main` can run ahead of the installed ESPHome: that is what
   broke `esphome config` for `jbdbms_v2` (`cv.sensitive`) before the 2026.10 upgrade.
6. **Classify each finding** and report as a table per device:
   - **Must fix**: the repo uses something removed or breaking.
   - **Verify on device**: behavior changed (defaults, timing, coexistence); needs a look
     after flashing.
   - **Opportunity**: a new feature that could replace custom code or fix a known pain.
   - **No action**: touches a component we have but not in a way we use it.
   Say which findings came from the changelog summary only and which you confirmed.
7. **Rework, only as far as the user asked.**
   - "What changed / review" -> report only, then offer the rework.
   - "Rework / apply" -> one device at a time, smallest edit per finding, repo conventions
     (shared `<device>-common.yaml`, thin YAML plus helper C++). Ask before replacing
     working custom code (for example `panic_capture.h`) with a built-in feature.
8. **Validate.**
   - `esphome config <yaml>` for every device touched, with output redirected to a file in
     the scratchpad and grep'd (it prints secrets). Compare with the baseline in
     `last-reviewed.md`, so already-broken files are not blamed on the change.
   - Do not run `esphome compile`/`run` in the user's device folder: they compile and
     flash, and two builds in one `.esphome/build/<name>` corrupt each other. If compile
     evidence is needed, compile a copy under the scratchpad and delete it afterwards
     (it holds `secrets.yaml`).
   - Config validation does not exercise lambdas, `.h` files or external-component C++.
     Say so, and list compile as not done. A cheap middle step on a scratchpad copy is
     `esphome compile --only-generate`: in `main.cpp`, any line that calls a helper from an
     `includes:` header **before** the `#include` line will not compile (2026.10 hoists templated
     apply-actions such as `servo.write` / `mqtt.publish` above the includes; found on
     `garage/generator`, which `esphome config` accepted).
9. **Update `references/last-reviewed.md`**: new reviewed version and date, the validation
   baseline, a short digest of findings, and what was applied, deferred or left open. Keep
   `references/project-map.md` current when a device or component is added.
10. **Mirror to `.github/skills/esphome-release-review/`** (same files, same content), like
    the other skills in this repo.

## Limits to state to the user
- The review is only as complete as the pages that were read; say which pages or offsets
  were not covered.
- A valid config is not a working device. List what still needs compile and a real run.
- "Verify on device" items mostly cannot be settled from the PC; give the user the concrete
  thing to watch (entity, log line, symptom). One read-only check is available:
  `esphome logs <yaml> --device <host-or-ip>` under `timeout` prints the running ESPHome
  version and chip revision (see `project-map.md`). Never upload or flash from here.
- A failed OTA after an upgrade (`Finishing update failed`, response 0x84) means the device
  rejected the image: compare the image header (`python -m esptool image-info <bin>`) with
  the chip revision from the log before suspecting anything else.

## Output
In this order: range reviewed, findings table per device (class + one line + PR number),
what was changed (if anything) and how it was validated, what the user must do (compile,
flash, watch), open items carried to the next review.
