# LogicPower UPS ESPHome gateway

ESP32 telemetry and control gateway for the LogicPower LPM-PSW-4500VA UPS using
the standard Megatec/Q1 RS232 protocol.

## Files

- `logicpower-ups-device.yaml`: main ESPHome entry file.
- `logicpower-ups-common.yaml`: shared WiFi, OTA, web, time, and diagnostics.
- `logicpower-ups-pipsolar.yaml`: active native ESPHome PIPSolar experiment.
- `logicpower-ups-megatec.yaml`: UART, Home Assistant entities, polling, and commands.
- `logicpower-ups-megatec.h`: Megatec response parser and command formatting.
- `logicpower.yaml`: compatibility entry that includes the main device file.

## Wiring

The ESP32 UART is configured as:

| ESP32 | MAX3232 TTL side |
|---|---|
| GPIO17 (TX) | RX / T1IN |
| GPIO16 (RX) | TX / R1OUT |
| GND | GND |

Connect the MAX3232 DB9 side to the UPS RS232 port with the cable orientation
required by the UPS. If no valid `Q1` response appears, verify whether pins 2 and
3 require a straight-through or null-modem connection.

The linked converter is advertised as a 5 V module. ESP32 GPIO is not 5 V
tolerant. Power the converter at 3.3 V only if the exact board supports it, or
use a level shifter/divider on the converter TX to ESP32 RX path. Measure the TTL
output before connecting it to GPIO16.

## Protocol

Serial settings are 2400 baud, 8 data bits, no parity, and 1 stop bit. The active
package uses ESPHome's native PIPSolar component and exposes its raw protocol
responses so compatibility can be confirmed from logs and Home Assistant.

The original Megatec/Q1 package remains available as a fallback. To restore it,
replace the `pipsolar` package include in `logicpower-ups-device.yaml` with the
`megatec` include and restore `logicpower-ups-megatec.h` under `esphome.includes`.

Home Assistant exposes all standard Megatec commands implemented by NUT's Q1
driver: beeper toggle, quick/timed/deep battery tests, test cancellation,
shutdown cancellation/load-on, delayed shutdown with return on mains, delayed
shutdown with timed restart, delayed shutdown that stays off, and immediate
load-off.

Shutdown and load commands can interrupt protected equipment. They are never
sent automatically; use their Home Assistant buttons deliberately.

## Validate

```powershell
esphome config .\logicpower-ups-device.yaml
esphome compile .\logicpower-ups-device.yaml
```