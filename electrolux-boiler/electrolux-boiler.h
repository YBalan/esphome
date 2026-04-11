#pragma once

#include "electrolux-boiler-defines.h"

// ─── Helpers ──────────────────────────────────────────────────────────────────

// Sum the first `count` bytes of `data` and return the result as a checksum byte.
static uint8_t boiler_calc_checksum(const std::vector<uint8_t>& data, size_t count) {
    uint8_t cs = 0;
    for (size_t i = 0; i < count && i < data.size(); i++) cs += data[i];
    return cs;
}

// Sum all bytes and return checksum (overload for full-packet TX use).
static uint8_t boiler_calc_checksum(const std::vector<uint8_t>& data) {
    return boiler_calc_checksum(data, data.size());
}

// Append the checksum byte (sum of all current bytes) to the packet in-place.
static void boiler_append_checksum(std::vector<uint8_t>& pkt) {
    pkt.push_back(boiler_calc_checksum(pkt));
}

// Serialise packet bytes to a colon-separated decimal string, e.g. "170:4:10:0".
static std::string boiler_bytes_to_dec_str(const std::vector<uint8_t>& data) {
    std::string s;
    for (size_t i = 0; i < data.size(); i++) {
        if (i > 0) s += ":";
        s += std::to_string(data[i]);
    }
    return s;
}

// Log an outgoing packet on TAG_TX in both HEX and decimal formats.
// `cs` is passed explicitly so callers without an appended checksum byte can still log 0.
static void boiler_log_tx(const std::vector<uint8_t>& pkt, uint8_t cs) {
    ESP_LOGW(TAG_TX, "HEX >>> %s, CS: %02X", format_hex_pretty(pkt).c_str(), cs);
    ESP_LOGW(TAG_TX, "DEC >>> %s, CS: %d",   boiler_bytes_to_dec_str(pkt).c_str(), cs);
}

// Log a received packet on TAG_RX in both HEX and decimal formats, including the checksum byte.
static void boiler_log_rx(const std::vector<uint8_t>& pkt, uint8_t cs) {
    ESP_LOGI(TAG_RX, "HEX <<< %s, CS: %02X", format_hex_pretty(pkt).c_str(), cs);
    ESP_LOGI(TAG_RX, "DEC <<< %s, CS: %d",   boiler_bytes_to_dec_str(pkt).c_str(), cs);
}

// ─── Temperature Trend ────────────────────────────────────────────────────────

// Push `cur_t` into a ring buffer and return "Up", "Down", or "Stable" using a
// linear-regression slope over the buffered samples.
// Returns nullptr until BOILER_TREND_MIN_SAMPLES readings have been collected.
// Up threshold varies by mode (faster heating → steeper slope expected).
// Down threshold is fixed — cooling rate is always passive and gradual.
static const char* boiler_calc_temp_trend(float cur_t, uint8_t mode) {
    static float buf[BOILER_TREND_MAX_SAMPLES] = {};
    static uint8_t pos   = 0;
    static uint8_t count = 0;

    buf[pos] = cur_t;
    pos = (pos + 1) % BOILER_TREND_MAX_SAMPLES;
    if (count < BOILER_TREND_MAX_SAMPLES) count++;

    if (count < BOILER_TREND_MIN_SAMPLES) return nullptr;

    // Reconstruct chronological order and compute least-squares slope.
    // x = sample index (0 = oldest), y = temperature value.
    float n = count;
    float sum_x = 0, sum_y = 0, sum_xy = 0, sum_xx = 0;
    for (uint8_t i = 0; i < count; i++) {
        float x = i;
        float y = buf[(pos - count + i + BOILER_TREND_MAX_SAMPLES) % BOILER_TREND_MAX_SAMPLES];
        sum_x  += x;
        sum_y  += y;
        sum_xy += x * y;
        sum_xx += x * x;
    }
    float denom = n * sum_xx - sum_x * sum_x;
    if (denom == 0) return BOILER_TREND_STR_STABLE;
    float slope = (n * sum_xy - sum_x * sum_y) / denom;

    const char* trend;
    auto it = BOILER_MODE_MAP.find(mode);
    float up_slope = (it != BOILER_MODE_MAP.end()) ? it->second.up_slope : BOILER_TREND_SLOPE_UP_LOW;
    if      (slope >  up_slope)                trend = BOILER_TREND_STR_UP;
    else if (slope < -BOILER_TREND_SLOPE_DOWN) trend = BOILER_TREND_STR_DOWN;
    else                                       trend = BOILER_TREND_STR_STABLE;

    // Build chronological buffer string for logging
    char buf_str[BOILER_TREND_MAX_SAMPLES * 8] = {};
    char* p = buf_str;
    for (uint8_t i = 0; i < count; i++) {
        float y = buf[(pos - count + i + BOILER_TREND_MAX_SAMPLES) % BOILER_TREND_MAX_SAMPLES];
        p += sprintf(p, i ? " %.1f" : "%.1f", y);
    }
    ESP_LOGW(TAG_TREND, "buf=[%s] slope=%.3f up_thr=%.2f down_thr=%.2f samples=%d trend=%s",
             buf_str, slope, up_slope, BOILER_TREND_SLOPE_DOWN, count, trend);
    return trend;
}

// ─── Main Functions ───────────────────────────────────────────────────────────

void boiler_parse_rx_packet(esphome::uart::UARTDirection direction, std::vector<uint8_t>& bytes) {
    UARTDebug::log_hex(direction, bytes, ':');

    if (bytes.size() < BOILER_RX_SIZE || bytes[0] != BOILER_HEADER || bytes[BOILER_RX_IDX_CMD] != BOILER_RX_CMD_STATUS)
        return;

    id(blink_led_script).execute();

    uint8_t checksum = boiler_calc_checksum(bytes, BOILER_RX_CS_COUNT);

    if (checksum != bytes[BOILER_RX_IDX_CS]) {
        ESP_LOGW(TAG_RX, "Checksum Error! Expected %02X (%d), got %02X (%d)",
                 checksum, checksum, bytes[BOILER_RX_IDX_CS], bytes[BOILER_RX_IDX_CS]);
        return;
    }

    float cur_t = bytes[BOILER_RX_IDX_CUR_T];
    float tar_t = bytes[BOILER_RX_IDX_TAR_T];
    id(current_temp).publish_state(cur_t);
    id(target_temp).publish_state(tar_t);

    id(global_boiler_target_temp) = (int)tar_t;
    id(boiler_target_temp_set).publish_state(tar_t);
    id(global_boiler_power_on) = (bytes[BOILER_RX_IDX_MODE] > BOILER_MODE_OFF &&
                           bytes[BOILER_RX_IDX_MODE] <= BOILER_MODE_NO_FROST);

    // Look up mode byte in the map; publish label and update level/select if applicable.
    auto it = BOILER_MODE_MAP.find(bytes[BOILER_RX_IDX_MODE]);
    const char* mode_label = (it != BOILER_MODE_MAP.end()) ? it->second.label : BOILER_STR_UNKNOWN;
    id(power_str).publish_state(mode_label);
    if (it != BOILER_MODE_MAP.end() && it->second.level >= 0) {
        id(global_boiler_power_level) = it->second.level;
        id(boiler_mode_switch).publish_state((uint8_t)it->second.sel_idx);
    }

    // Trend uses global_boiler_power_level (last known commanded level) rather than the
    // raw RX mode byte, because the boiler can report 0x00 in its status packet even
    // while actively heating (e.g. mid-cycle). The global is already updated above.
    const char* trend = boiler_calc_temp_trend(cur_t, (uint8_t)id(global_boiler_power_level));
    if (trend) id(temp_trend).publish_state(trend);

    id(boiler_switch).publish_state(id(global_boiler_power_on));

    // Bacteria Stop Technology
    id(bst_active).publish_state(bytes[BOILER_RX_IDX_BST] == BOILER_BST_ACTIVE);

    // Smart Heating Detector: heating when cur_t is below tar_t by threshold
    id(is_heating).publish_state(cur_t <= (tar_t - BOILER_HEAT_THRESHOLD) && id(global_boiler_power_on) == true);

    // Boiler internal clock as human-readable HH:MM string
    char time_buf[6];
    sprintf(time_buf, "%02d:%02d", bytes[BOILER_RX_IDX_UP_H], bytes[BOILER_RX_IDX_UP_M]);
    id(boiler_clock).publish_state(time_buf);

    // Sync timer start time from boiler state and update the number inputs in HA.
    id(global_timer_hours)   = bytes[BOILER_RX_IDX_TMR_H];
    id(global_timer_minutes) = bytes[BOILER_RX_IDX_TMR_M];
    id(boiler_timer_hours_set).publish_state(bytes[BOILER_RX_IDX_TMR_H]);
    id(boiler_timer_minutes_set).publish_state(bytes[BOILER_RX_IDX_TMR_M]);
    sprintf(time_buf, "%02d:%02d", bytes[BOILER_RX_IDX_TMR_H], bytes[BOILER_RX_IDX_TMR_M]);
    id(timer_human).publish_state(time_buf);

    std::string raw_string = boiler_bytes_to_dec_str(bytes);
    
    boiler_log_rx(bytes, bytes[BOILER_RX_IDX_CS]);  // Log RX packet on RX tag for easy comparison with outgoing packets
    id(raw_packet_history).publish_state(raw_string);
}

// Find the mode map entry whose label matches `x` and apply its level to the global.
void boiler_set_power_level(const std::string& x) {
    for (const auto& entry : BOILER_MODE_MAP) {
        if (entry.second.label == x && entry.second.level >= 0) {
            id(global_boiler_power_level) = entry.second.level;
            return;
        }
    }
}

// Return the nominal power (W) for the current global_boiler_power_level.
// The level value equals the RX mode byte, which is the map key.
static int boiler_get_power_watts() {
    auto it = BOILER_MODE_MAP.find((uint8_t)id(global_boiler_power_level));
    return (it != BOILER_MODE_MAP.end()) ? it->second.power_w : 0;
}

std::vector<uint8_t> boiler_build_tx_packet() {
    std::vector<uint8_t> pkt = {
        BOILER_HEADER,
        BOILER_TX_LEN_POWER,
        BOILER_TX_CMD_WRITE,
        BOILER_SUBCMD_POWER,
        (uint8_t)(id(global_boiler_power_on) ? (uint8_t)id(global_boiler_power_level) : BOILER_MODE_OFF),
        (uint8_t)id(global_boiler_target_temp),
    };
    boiler_append_checksum(pkt);
    boiler_log_tx(pkt, pkt.back());
    return pkt;
}


std::vector<uint8_t> boiler_build_bst_packet(bool on) {
    std::vector<uint8_t> pkt = {
        BOILER_HEADER,
        BOILER_TX_LEN_BST,
        BOILER_TX_CMD_WRITE,
        BOILER_SUBCMD_BST,
        (uint8_t)(on ? BOILER_BST_ACTIVE : BOILER_BST_INACTIVE),
    };
    boiler_append_checksum(pkt);
    boiler_log_tx(pkt, pkt.back());
    return pkt;
}

// Build the timer packet: arms the boiler to start heating at the specified clock time (HH:MM).
// The boiler compares HH:MM against its internal clock (synced via boiler_build_clock_packet).
// ref: https://github.com/dentra/esphome-ewh/blob/master/reverse.md — subcmd 0x02
// ref: https://github.com/dentra/esphome-ewh/blob/master/components/ewh/ewh_data.h — ewh_timer_t
// Format: AA [len=6] 0A 02 [HH] [MM] [MODE] [TEMP] [CS]
std::vector<uint8_t> boiler_build_timer_packet() {
    std::vector<uint8_t> pkt = {
        BOILER_HEADER,
        BOILER_TX_LEN_TIMER,
        BOILER_TX_CMD_WRITE,
        BOILER_SUBCMD_TIMER,
        (uint8_t)id(global_timer_hours),
        (uint8_t)id(global_timer_minutes),
        BOILER_MODE_LOW, //Only BOILER_MODE_LOW for timer, Power level not supported: (uint8_t)id(global_boiler_power_level),  // heating mode to use when timer fires
        (uint8_t)id(global_boiler_target_temp),  // target temperature when timer fires
    };
    boiler_append_checksum(pkt);
    boiler_log_tx(pkt, pkt.back());
    return pkt;
}

// Request the boiler's current state immediately (response: RX state packet cmd=0x09/0x88).
// Useful to confirm that a write command (timer, mode, etc.) was accepted.
// ref: https://github.com/dentra/esphome-ewh/blob/master/reverse.md — cmd 0x08
// Format: AA [len=3] 08 10 04 [CS]
std::vector<uint8_t> boiler_build_state_request() {
    std::vector<uint8_t> pkt = {
        BOILER_HEADER,
        BOILER_TX_LEN_STATE_REQ,
        BOILER_TX_CMD_STATE_REQ,
        0x10, 0x04,  // fixed payload from reverse.md
    };
    boiler_append_checksum(pkt);
    boiler_log_tx(pkt, pkt.back());
    return pkt;
}

// Build the clock-sync packet from the current NTP time.
// Returns an empty vector if time is not yet valid.
std::vector<uint8_t> boiler_build_clock_packet() {
    auto now = id(sntp_time).now();
    if (!now.is_valid()) {
        ESP_LOGW(TAG_CLOCK, "Time not valid yet, skipping sync...");
        return {};
    }

    std::vector<uint8_t> pkt = {
        BOILER_HEADER,
        BOILER_TX_LEN_TIME,
        BOILER_TX_CMD_WRITE,
        BOILER_SUBCMD_TIME,
        (uint8_t)now.hour,
        (uint8_t)now.minute,
    };
    boiler_append_checksum(pkt);

    ESP_LOGI(TAG_CLOCK, "Boiler time synced to %02d:%02d", now.hour, now.minute);
    return pkt;
}

// Parse a raw command string into a byte payload, optionally appending a checksum.
// Prefix "C:" or "c:" triggers automatic checksum append.
// Returns the built payload; also publishes state and logs the input.
std::vector<uint8_t> boiler_build_raw_payload(const std::string& x) {
    std::string input = x;  // local copy — prefix may be stripped below
    id(raw_cmd_input).publish_state(x);
    ESP_LOGW(TAG_TX, "RAW Inp: %s", x.c_str());

    bool add_checksum = (input.size() >= 2 && (input.substr(0, 2) == "C:" || input.substr(0, 2) == "c:"));
    if (add_checksum) input = input.substr(2);

    // Parse colon-separated decimal string into bytes
    std::vector<uint8_t> payload;
    char* buf = strdup(input.c_str());
    for (char* tok = strtok(buf, ":"); tok != NULL; tok = strtok(NULL, ":"))
        payload.push_back((uint8_t)atoi(tok));
    free(buf);

    if (payload.empty()) {
        ESP_LOGW(TAG_TX, "Invalid input format! Use dec:dec:dec");
        return payload;
    }

    uint8_t cs = 0;
    if (add_checksum) {
        cs = boiler_calc_checksum(payload);
        payload.push_back(cs);
    }

    boiler_log_tx(payload, cs);
    return payload;
}

