#pragma once

// https://github.com/dentra/esphome-ewh

#include <map>

// ─── Protocol: Packet Header ─────────────────────────────────────────────────
#define BOILER_HEADER           0xAA  // First byte of every packet (decimal: 170)

// ─── Protocol: RX Packet (Status update, 13 bytes) ───────────────────────────
#define BOILER_RX_SIZE          13    // Total expected RX packet length in bytes
#define BOILER_RX_CMD_STATUS    0x09  // Command-type byte value that marks a status packet
#define BOILER_RX_CS_COUNT      12    // Number of bytes summed for checksum (bytes 0–11)

// RX packet byte indices
#define BOILER_RX_IDX_CMD       2     // Command type
#define BOILER_RX_IDX_MODE      3     // Active power mode
#define BOILER_RX_IDX_CUR_T     4     // Current water temperature (°C)
#define BOILER_RX_IDX_TAR_T     5     // Target water temperature (°C)
#define BOILER_RX_IDX_UP_H      6     // Uptime hours
#define BOILER_RX_IDX_UP_M      7     // Uptime minutes
#define BOILER_RX_IDX_TMR_H     8     // Countdown timer hours
#define BOILER_RX_IDX_TMR_M     9     // Countdown timer minutes
#define BOILER_RX_IDX_BST       11    // Bacteria Stop Technology active flag
#define BOILER_RX_IDX_CS        12    // Packet checksum

// ─── Protocol: TX Packet ─────────────────────────────────────────────────────
#define BOILER_TX_CMD_WRITE     10    // Command type for all write packets (0x0A)
#define BOILER_TX_LEN_POWER     4     // Length field value for power/temperature packets
#define BOILER_TX_LEN_BST       3     // Length field value for BST on/off packets
#define BOILER_TX_LEN_TIME      4     // Length field value for clock-sync packets

// TX sub-command values (byte index 3 of every TX packet)
#define BOILER_SUBCMD_POWER     0     // Set power mode and target temperature
#define BOILER_SUBCMD_TIME      1     // Set the boiler's internal clock
#define BOILER_SUBCMD_TIMER     2     // Set the countdown timer
#define BOILER_SUBCMD_BST       3     // Toggle Bacteria Stop Technology

// ─── Protocol: Power Mode Byte Values (RX byte 3 / TX byte 4) ────────────────
#define BOILER_MODE_OFF         0x00  // Boiler is switched off
#define BOILER_MODE_LOW         0x01  // 700 W — low-power heating
#define BOILER_MODE_MEDIUM      0x02  // 1300 W — medium-power heating
#define BOILER_MODE_HIGH        0x03  // 2000 W — full-power heating
#define BOILER_MODE_TIMER       0x04  // Timer-controlled mode (read-only from boiler)
#define BOILER_MODE_NO_FROST    0x05  // Anti-frost mode — maintains 5 °C

// ─── Protocol: BST Byte Values ───────────────────────────────────────────────
#define BOILER_BST_ACTIVE       0x01  // BST mode is on
#define BOILER_BST_INACTIVE     0x00  // BST mode is off

// ─── Internal: Heating Detection Threshold ───────────────────────────────────
#define BOILER_HEAT_THRESHOLD   2     // °C below target at which active heating is reported

// ─── Log Tags ────────────────────────────────────────────────────────────────
#define TAG_RX      "boiler_rx"   // UART receive: packet parsing and sensor updates
#define TAG_TX      "boiler_tx"   // UART transmit: outgoing command packets
#define TAG_CLOCK   "clock_sync"  // NTP-to-boiler clock synchronisation

// ─── Mode Map ─────────────────────────────────────────────────────────────────
// Holds all per-mode data in one place: display label, internal power level,
// and select-component option index.
// level == -1  → mode has no selectable power level (OFF, Timer)
// sel_idx == -1 → mode does not appear in the select component
struct BoilerModeInfo {
    const char* label;  // Human-readable string published to HA
    int level;          // Value written to global_boiler_power_level global; -1 = not applicable
    int sel_idx;        // boiler_mode_switch option index; -1 = not in the select list
};

// Fallback label for any mode byte not present in the map
#define BOILER_STR_UNKNOWN  "Unknown"

static const std::map<uint8_t, BoilerModeInfo> BOILER_MODE_MAP = {
    { BOILER_MODE_OFF,      { "Power OFF",      -1,  -1 } },  // boiler is off
    { BOILER_MODE_LOW,      { "700W (Low)",       1,   0 } },  // 700 W,  level=1, sel=0
    { BOILER_MODE_MEDIUM,   { "1300W (Medium)",   2,   1 } },  // 1300 W, level=2, sel=1
    { BOILER_MODE_HIGH,     { "2000W (High)",     3,   2 } },  // 2000 W, level=3, sel=2
    { BOILER_MODE_TIMER,    { "Timer",           -1,  -1 } },  // timer: read-only, not in select
    { BOILER_MODE_NO_FROST, { "No Frost 5°C",    5,   3 } },  // anti-frost 5 °C, level=5, sel=3
};
