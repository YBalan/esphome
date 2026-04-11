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
#define BOILER_RX_IDX_UP_H      6     // Boiler clock hours
#define BOILER_RX_IDX_UP_M      7     // Boiler clock minutes
#define BOILER_RX_IDX_TMR_H     8     // Scheduled timer start hour (set by subcmd 0x02)
#define BOILER_RX_IDX_TMR_M     9     // Scheduled timer start minute (set by subcmd 0x02)
#define BOILER_RX_IDX_BST       11    // Bacteria Stop Technology active flag
#define BOILER_RX_IDX_CS        12    // Packet checksum

// ─── Protocol: TX Packet ─────────────────────────────────────────────────────
#define BOILER_TX_CMD_WRITE     10    // Command type for all write packets (0x0A)
#define BOILER_TX_CMD_STATE_REQ 0x08  // Command type for requesting current device state
#define BOILER_TX_LEN_POWER     4     // Length field value for power/temperature packets
#define BOILER_TX_LEN_BST       3     // Length field value for BST on/off packets
#define BOILER_TX_LEN_TIME      4     // Length field value for clock-sync packets
#define BOILER_TX_LEN_TIMER     6     // Length field value for timer packets (HH + MM + MODE + TEMP)
#define BOILER_TX_LEN_STATE_REQ 3     // Length field value for state-request packet (cmd + 2 fixed bytes)

// TX sub-command values (byte index 3 of every TX packet)
#define BOILER_SUBCMD_POWER     0     // Set power mode and target temperature
#define BOILER_SUBCMD_TIME      1     // Set the boiler's internal clock
#define BOILER_SUBCMD_TIMER     2     // Set the timer
#define BOILER_SUBCMD_BST       3     // Toggle Bacteria Stop Technology

// ─── Protocol: Power Mode Byte Values (RX byte 3 / TX byte 4) ────────────────
#define BOILER_MODE_OFF         0x00  // Boiler is switched off
#define BOILER_MODE_LOW         0x01  // 700 W — low-power heating
#define BOILER_MODE_MEDIUM      0x02  // 1300 W — medium-power heating
#define BOILER_MODE_HIGH        0x03  // 2000 W — full-power heating
#define BOILER_MODE_TIMER       0x04  // Timer-controlled mode — boiler is idle until scheduled start time, then heats at 700W
#define BOILER_MODE_NO_FROST    0x05  // Anti-frost mode — maintains 5 °C

// ─── Protocol: BST Byte Values ───────────────────────────────────────────────
#define BOILER_BST_ACTIVE       0x01  // BST mode is on
#define BOILER_BST_INACTIVE     0x00  // BST mode is off

// ─── Internal: Heating Detection Threshold ───────────────────────────────────
#define BOILER_HEAT_THRESHOLD           2       // °C below target at which active heating is reported

// ─── Internal: Energy Accumulation ───────────────────────────────────────────
#define BOILER_ENERGY_INTERVAL_S        30.0f   // Accumulation cadence in seconds — must match energy_interval substitution in YAML
#define BOILER_WH_PER_INTERVAL(w)       ((w) * (BOILER_ENERGY_INTERVAL_S / 3600.0f))  // Wh added per tick at power w
#define BOILER_WH_TO_KWH                1000.0f // Divisor to convert Wh accumulator to kWh for publishing

// ─── Internal: Temperature Trend Labels ──────────────────────────────────────
#define BOILER_TREND_STR_UP     "Up"
#define BOILER_TREND_STR_DOWN   "Down"
#define BOILER_TREND_STR_STABLE "Stable"

// ─── Internal: Temperature Trend Samples ─────────────────────────────────────
#define BOILER_TREND_MIN_SAMPLES        2       // Minimum readings before a trend is reported
#define BOILER_TREND_MAX_SAMPLES        10       // Ring-buffer capacity for linear regression
#define BOILER_TREND_SLOPE_DOWN         0.10f   // °C/sample — slope threshold to classify Down (cooling is always gradual)
// #define BOILER_TREND_SLOPE_UP_PASSIVE   0.10f   // °C/sample Up threshold for OFF / No Frost — no active or intermittent heating
#define BOILER_TREND_SLOPE_UP_LOW       0.20f   // °C/sample Up threshold for 700 W — slow heating rate
// #define BOILER_TREND_SLOPE_UP_MEDIUM    0.35f   // °C/sample Up threshold for 1300 W — moderate heating rate
// #define BOILER_TREND_SLOPE_UP_HIGH      0.50f   // °C/sample Up threshold for 2000 W — fast heating rate

// // ─── Internal: Nominal Power Draw per Mode ────────────────────────────────────
// #define BOILER_POWER_W_OFF              0       // Watts consumed when off
// #define BOILER_POWER_W_LOW              700     // Watts consumed in Low mode
// #define BOILER_POWER_W_MEDIUM           1300    // Watts consumed in Medium mode
// #define BOILER_POWER_W_HIGH             2000    // Watts consumed in High mode

// ─── Internal: Mode Display Labels ───────────────────────────────────────────
#define BOILER_STR_MODE_OFF             "Power OFF"
#define BOILER_STR_MODE_LOW             "700W (Low)"
#define BOILER_STR_MODE_MEDIUM          "1300W (Medium)"
#define BOILER_STR_MODE_HIGH            "2000W (High)"
#define BOILER_STR_MODE_TIMER           "Timer"
#define BOILER_STR_MODE_NO_FROST        "No Frost 5°C"

// ─── Internal: boiler_mode_switch Select Option Indices ───────────────────────
#define BOILER_SEL_NONE                 -1  // Mode does not appear in the select component
#define BOILER_SEL_LOW                   0
#define BOILER_SEL_MEDIUM                1
#define BOILER_SEL_HIGH                  2
#define BOILER_SEL_TIMER                 3
#define BOILER_SEL_NO_FROST              4

// ─── Log Tags ────────────────────────────────────────────────────────────────
#define TAG_RX      "boiler_rx"   // UART receive: packet parsing and sensor updates
#define TAG_TX      "boiler_tx"   // UART transmit: outgoing command packets
#define TAG_CLOCK   "clock_sync"  // NTP-to-boiler clock synchronisation
#define TAG_TREND   "temp_trend"  // Temperature trend calculation

// ─── Mode Map ─────────────────────────────────────────────────────────────────
// Holds all per-mode data in one place: display label, internal power level,
// select-component option index, nominal power draw, and Up-trend slope threshold.
// level == -1     → mode has no selectable power level (OFF only)
// sel_idx == -1   → mode does not appear in the select component
// up_slope        → °C/sample slope needed to call "Up"; higher wattage heats faster so needs a steeper threshold
struct BoilerModeInfo {
    const char* label;  // Human-readable string published to HA
    int level;          // Value written to global_boiler_power_level global; -1 = not applicable
    int sel_idx;        // boiler_mode_switch option index; -1 = not in the select list
    int power_w;        // Nominal power draw in watts when heating; 0 = not applicable
    float up_slope;     // °C/sample slope threshold to classify "Up" — faster modes heat steeper
};

// Fallback label for any mode byte not present in the map
#define BOILER_STR_UNKNOWN  "Unknown"

static const std::map<uint8_t, BoilerModeInfo> BOILER_MODE_MAP = {
    //                           label               level  sel_idx  power_w  up_slope
    { BOILER_MODE_OFF,      { BOILER_STR_MODE_OFF,         -1,    -1,        0,    0.00f } },  // off — no active heating; treat any tiny slope as stable
    { BOILER_MODE_LOW,      { BOILER_STR_MODE_LOW,         0x01,   0,      700,    0.00f } },  // 700 W — slow heating rate
    { BOILER_MODE_MEDIUM,   { BOILER_STR_MODE_MEDIUM,      0x02,   1,     1300,    0.00f } },  // 1300 W — moderate heating rate
    { BOILER_MODE_HIGH,     { BOILER_STR_MODE_HIGH,        0x03,   2,     2000,    0.00f } },  // 2000 W — fast heating rate
    { BOILER_MODE_TIMER,    { BOILER_STR_MODE_TIMER,       0x04,   3,        0,    0.00f } },  // timer mode: heats at 700 W when active; TX via subcmd 0x02
    { BOILER_MODE_NO_FROST, { BOILER_STR_MODE_NO_FROST,    0x04,   4,      700,    0.00f } },  // intermittent 700 W to hold 5 °C; very slow rise expected
};
