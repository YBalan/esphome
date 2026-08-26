# LogicPower UPS ESPHome gateway

ESP32 telemetry and control gateway for the LogicPower LPM-PSW-4500VA UPS using
the standard Megatec/Q1 RS232 protocol.

## Files

- `logicpower-ups-device.yaml`: main ESPHome entry file.
- `logicpower-ups-common.yaml`: shared WiFi, OTA, web, time, and diagnostics.
- `logicpower-ups-pipsolar.yaml`: inactive native ESPHome PIPSolar experiment.
- `logicpower-ups-syssi-pip8048.yaml`: isolated syssi `pip8048` telemetry package.
- `logicpower-ups-megatec.yaml`: active UART, Home Assistant entities, polling, and commands.
- `logicpower-ups-megatec.h`: Megatec response parser and command formatting.
- `logicpower-pip8048.yaml`: standalone entry for the syssi `pip8048` experiment.
- `logicpower-protocol-test.yaml`: read-only PI30/PI18/PI17/PI16/Megatec protocol probe.
- `logicpower-powermanager.yaml` / `logicpower-powermanager.h`: standalone raw RS232 comport
  monitor + manual command sender — see "PowerManager comport monitor" below.
- `logicpower.yaml`: compatibility entry that includes the main device file.

## Wiring

The ESP32 UART is configured as:

| ESP32 | MAX3232 TTL side |
|---|---|
| GPIO17 (TX) | RX / T1IN |
| GPIO16 (RX) | TX / R1OUT |
| GND | GND |

The published Megatec RS232 cable uses non-PC DB9 pins: UPS pin 9 is TX to the
computer, pin 6 is RX from the computer, and pin 7 is ground. Verify this pinout
against the exact UPS hardware before connecting it; a standard 2/3/5 serial
cable is not the documented Megatec cable.

If the RX log contains only exact copies of transmitted commands such as `Q1`,
`F`, and `I`, isolate the loopback by disconnecting the cable at the UPS and
checking the log again. If echoes remain, the MAX3232 module or cable loops TX
back to RX. If echoes stop, inspect the UPS-side 6/7/9 wiring before changing
the protocol or baud rate.

The full protocol probe was run twice at 2400 8N1. Every PI30, PI18, PI17,
PI16, and Megatec query appeared on RX as an immediate byte-for-byte copy of TX,
with no additional or delayed response. This result cannot identify a supported
protocol because no UPS response reached the ESP32. A valid response would be a
different frame, commonly beginning with `(`, `#`, or `^`.

To locate the echo, leave the ESP32 and MAX3232 powered and run the probe after
unplugging the DB9 connector from the UPS. If echoes remain, disconnect the DB9
cable from the MAX3232 as well. Echoes with the RS232 cable completely removed
indicate MAX3232/TTL wiring or hardware; echoes only with the cable attached
indicate a cable pin bridge or UPS-side behavior. Do not select another protocol
until this test produces silence or a non-echo response.

The linked converter is advertised as a 5 V module. ESP32 GPIO is not 5 V
tolerant. Power the converter at 3.3 V only if the exact board supports it, or
use a level shifter/divider on the converter TX to ESP32 RX path. Measure the TTL
output before connecting it to GPIO16.

## Protocol

The published Megatec serial settings are 2400 baud, 8 data bits, no parity,
and 1 stop bit. A runtime test at 9600 baud still returned exact local command
echoes and no telemetry, so the active profile remains at 2400 baud. PIPSolar
queries also produced no response data, so that package remains inactive.

Two PIPSolar alternatives remain available without changing the active Megatec
node. `logicpower-ups-pipsolar.yaml` uses ESPHome's native component, while
`logicpower-pip8048.yaml` is a standalone node using syssi's external `pip8048`
component. The external component is pinned to commit `322e2b2`, immediately
before its ESPHome 2026.3.0 minimum-version change, for compatibility with the
installed ESPHome 2026.2.4 toolchain. It exposes read-only telemetry and raw
protocol responses; command entities are intentionally omitted until valid UPS
responses are confirmed.

Home Assistant exposes all standard Megatec commands implemented by NUT's Q1
driver: beeper toggle, quick/timed/deep battery tests, test cancellation,
shutdown cancellation/load-on, delayed shutdown with return on mains, delayed
shutdown with timed restart, delayed shutdown that stays off, and immediate
load-off.

Shutdown and load commands can interrupt protected equipment. They are never
sent automatically; use their Home Assistant buttons deliberately.

## PowerManager comport monitor

LogicPower's Windows app for this UPS ("PowerManager" / "PowerManager II", downloadable from the
product page) does not have a published byte-level protocol spec, and no public reverse-engineering
of it was found. Rather than guess a specific dialect (Megatec, PI16/PI30/PI17/PI18 - all already
probed by `logicpower-protocol-test.yaml`), `logicpower-powermanager.yaml` is a minimal, protocol-
agnostic RS232 comport monitor: it does no parsing at all, it just makes the wire directly visible
and drivable from Home Assistant.

`logicpower-powermanager.yaml` is a standalone flashable node (own `device_name`/AP, reuses only
`logicpower-ups-common.yaml`) that:

- Logs every RX and TX frame at `ESP_LOGI` (visible without raising the logger level) and mirrors
  the latest frame into `Last RX Raw (hex)`/`Last RX ASCII`/`Last TX Raw (hex)`/`Last TX ASCII`
  text sensors, so traffic is visible directly in Home Assistant, not just in `esphome logs`.
- Sends nothing on its own - there is no automatic polling, so it won't interfere with whatever
  you're doing manually (running the vendor PowerManager app on a tapped/shared line, or probing
  commands by hand).
- Adds a `Raw Command` text field plus `Send Raw Command` button for sending arbitrary bytes from
  Home Assistant without reflashing: plain text (e.g. `QPIGS`) is sent with a trailing CR appended;
  `crc:QPIGS` sends it as ASCII with a computed Voltronic-style CRC-16 + CR appended (handy if you
  want to hand-test a CRC-framed command); `hex:51 50 49 0D` sends the exact raw bytes given.
  The CRC-16 implementation was verified byte-for-byte against the known-good `QPI`/`QMOD`/`QPIGS`/
  `QPIRI`/`QPIWS` command bytes already hardcoded in `logicpower-protocol-test.yaml`.

**Read this before spending time sending commands.** The prior probing already run twice via
`logicpower-protocol-test.yaml` found that *every* command in *every* dialect tried (Megatec, PI16,
PI30, PI17, PI18) came back on RX as an immediate, byte-for-byte copy of what was just transmitted,
with no additional or delayed data. That pattern - literally everything echoing, with zero genuine
replies across five unrelated protocol families - is far more consistent with a hardware TX→RX
loopback (in the MAX3232 module, the cable, or a DB9 pin bridge) than with a protocol mismatch.
With this monitor, that will show up as `Last RX Raw (hex)` being identical to `Last TX Raw (hex)`
every time, no matter what you send. Follow the loopback isolation steps earlier in this README
(unplug the DB9 from the UPS, then from the MAX3232) before spending more time trying commands -
nothing will produce a real reply until genuine, non-echoed bytes show up on RX.

## Validate

```powershell
esphome config .\logicpower-ups-device.yaml
esphome compile .\logicpower-ups-device.yaml

esphome config .\logicpower-pip8048.yaml
esphome compile .\logicpower-pip8048.yaml

esphome config .\logicpower-powermanager.yaml
esphome compile .\logicpower-powermanager.yaml
```