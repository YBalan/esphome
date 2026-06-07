#pragma once

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <limits>
#include <string>
#include <vector>

#include "esphome/components/remote_base/rc_switch_protocol.h"
#include "esphome/components/wifi/wifi_component.h"
#include "esphome/core/helpers.h"

namespace garage {

inline constexpr const char *kRfCodeNotAssigned = "Not assigned";

inline bool is_generator_running(const float &voltage, const float &threshold) {
  if (!std::isfinite(voltage)) {
    return false;
  }
  return voltage >= threshold;
}

inline const char *on_off_text(const bool &value) {
  return value ? "ON" : "OFF";
}

inline uint32_t now_ms() {
  return millis();
}

inline bool time_until_in_future(const uint32_t &timestamp_ms) {
  return static_cast<int32_t>(timestamp_ms - now_ms()) > 0;
}

inline void wake_backlight(const uint32_t &timeout_ms = 60000U) {
  id(display_backlight_until_ms) = now_ms() + timeout_ms;
}

inline bool is_backlight_active() {
  return time_until_in_future(id(display_backlight_until_ms));
}

inline void set_lcd_message(const std::string &line_1, const std::string &line_2, const uint32_t &duration_ms = 5000U) {
  id(lcd_message_line_1) = line_1;
  id(lcd_message_line_2) = line_2;
  id(lcd_message_until_ms) = now_ms() + duration_ms;
}

inline void set_lcd_message(const char *line_1, const char *line_2, const uint32_t &duration_ms = 5000U) {
  id(lcd_message_line_1) = line_1;
  id(lcd_message_line_2) = line_2;
  id(lcd_message_until_ms) = now_ms() + duration_ms;
}

inline void clear_lcd_message() {
  id(lcd_message_line_1).clear();
  id(lcd_message_line_2).clear();
  id(lcd_message_until_ms) = 0U;
}

inline bool is_lcd_message_active() {
  return time_until_in_future(id(lcd_message_until_ms));
}

inline void cycle_lcd_line2_mode() {
  // Two manual pages: 0 => voltage/power, 1 => current/power factor.
  id(lcd_line2_mode) = (id(lcd_line2_mode) + 1) % 2;
}

inline void copy_16(char (&buffer)[17], const std::string &value) {
  std::snprintf(buffer, sizeof(buffer), "%.*s", 16, value.c_str());
}

inline bool is_ap_mode_active() {
  return wifi::global_wifi_component != nullptr && wifi::global_wifi_component->is_ap_active();
}

inline void active_ap_ssid_or_default(char (&buffer)[17]) {
  if (wifi::global_wifi_component == nullptr) {
    std::snprintf(buffer, sizeof(buffer), "%s", "AP");
    return;
  }

  const char *ssid = wifi::global_wifi_component->get_ap().get_ssid().c_str();
  if (ssid == nullptr || ssid[0] == '\0') {
    std::snprintf(buffer, sizeof(buffer), "%s", "AP");
    return;
  }

  std::snprintf(buffer, sizeof(buffer), "%.*s", 16, ssid);
}

inline std::string u64_to_binary(const uint64_t &value) {
  if (value == 0ULL) {
    return "0";
  }

  std::string bits;
  bits.reserve(64U);
  bool started = false;
  for (int bit = 63; bit >= 0; --bit) {
    const bool on = ((value >> bit) & 1ULL) != 0ULL;
    if (!started && !on) {
      continue;
    }
    started = true;
    bits.push_back(on ? '1' : '0');
  }
  return bits;
}

inline uint32_t seconds_to_ms(const float &seconds) {
  if (!std::isfinite(seconds) || seconds <= 0.0f) {
    return 0U;
  }
  constexpr float kMaxSeconds = static_cast<float>(std::numeric_limits<uint32_t>::max()) / 1000.0f;
  if (seconds >= kMaxSeconds) {
    return std::numeric_limits<uint32_t>::max();
  }
  return static_cast<uint32_t>(seconds * 1000.0f + 0.5f);
}

template <typename TString>
inline bool has_text(const TString &text) {
  return !text.empty();
}

template <typename TString>
inline const TString &text_value(const TString &text) {
  return text;
}

// Map the generic button index to the matching relay pulse timeout.
inline float button_timeout_seconds(const uint8_t button) {
  switch (button) {
    case 1:
      return id(relay_1_timeout_s);
    case 2:
      return id(relay_2_timeout_s);
    case 3:
      return id(relay_3_timeout_s);
    case 4:
      return id(relay_4_timeout_s);
    default:
      return 0.0f;
  }
}

inline void set_button_timeout_seconds(const uint8_t button, const float &seconds) {
  switch (button) {
    case 1:
      id(relay_1_timeout_s) = seconds;
      break;
    case 2:
      id(relay_2_timeout_s) = seconds;
      break;
    case 3:
      id(relay_3_timeout_s) = seconds;
      break;
    case 4:
      id(relay_4_timeout_s) = seconds;
      break;
    default:
      break;
  }
}

inline bool button_timeout_is_infinite(const uint8_t button) {
  return button_timeout_seconds(button) < 0.0f;
}

// Convert the configured button timeout into ESPHome delay milliseconds.
inline uint32_t button_timeout_ms(const uint8_t button) {
  return seconds_to_ms(button_timeout_seconds(button));
}

// Map the generic button index to the matching learned RF code.
inline const std::string &button_rf_code(const uint8_t button) {
  static const std::string empty_code;

  switch (button) {
    case 1:
      return id(rf_code_btn_1);
    case 2:
      return id(rf_code_btn_2);
    case 3:
      return id(rf_code_btn_3);
    case 4:
      return id(rf_code_btn_4);
    default:
      return empty_code;
  }
}

inline std::string button_rf_code_or_default(const uint8_t button, const char *fallback = kRfCodeNotAssigned) {
  const std::string &code = button_rf_code(button);
  if (code.empty()) {
    return std::string(fallback);
  }
  return code;
}

inline std::string button_rf_raw_text(const uint8_t button) {
  std::string value;

  switch (button) {
    case 1:
      value = id(rf_raw_btn_1);
      value += id(rf_raw_btn_1_b);
      value += id(rf_raw_btn_1_c);
      break;
    case 2:
      value = id(rf_raw_btn_2);
      value += id(rf_raw_btn_2_b);
      value += id(rf_raw_btn_2_c);
      break;
    case 3:
      value = id(rf_raw_btn_3);
      value += id(rf_raw_btn_3_b);
      value += id(rf_raw_btn_3_c);
      break;
    case 4:
      value = id(rf_raw_btn_4);
      value += id(rf_raw_btn_4_b);
      value += id(rf_raw_btn_4_c);
      break;
    default:
      break;
  }

  return value;
}

inline void set_button_rf_raw_text(const uint8_t button, const std::string &raw_text) {
  constexpr size_t kChunkLen = 254U;
  const std::string part_a = raw_text.substr(0U, kChunkLen);
  const std::string part_b = (raw_text.size() > kChunkLen) ? raw_text.substr(kChunkLen, kChunkLen) : std::string();
  const std::string part_c = (raw_text.size() > (kChunkLen * 2U)) ? raw_text.substr(kChunkLen * 2U, kChunkLen) : std::string();

  switch (button) {
    case 1:
      id(rf_raw_btn_1) = part_a;
      id(rf_raw_btn_1_b) = part_b;
      id(rf_raw_btn_1_c) = part_c;
      break;
    case 2:
      id(rf_raw_btn_2) = part_a;
      id(rf_raw_btn_2_b) = part_b;
      id(rf_raw_btn_2_c) = part_c;
      break;
    case 3:
      id(rf_raw_btn_3) = part_a;
      id(rf_raw_btn_3_b) = part_b;
      id(rf_raw_btn_3_c) = part_c;
      break;
    case 4:
      id(rf_raw_btn_4) = part_a;
      id(rf_raw_btn_4_b) = part_b;
      id(rf_raw_btn_4_c) = part_c;
      break;
    default:
      break;
  }
}

inline bool button_rf_raw_enabled(const uint8_t button) {
  return has_text(button_rf_raw_text(button));
}

inline bool pending_rf_raw_enabled() {
  return has_text(id(rf_pending_raw));
}

inline bool button_rf_enabled(const uint8_t button) {
  return button_rf_raw_enabled(button);
}

inline bool is_rf_learning_mode() {
  return id(rf_learning_mode);
}

template <typename TTiming>
inline std::string raw_timings_to_text(const std::vector<TTiming> &timings) {
  std::string raw_text;
  raw_text.reserve(timings.size() * 10U);

  for (size_t i = 0; i < timings.size(); i++) {
    if (i != 0U) {
      raw_text += ", ";
    }
    raw_text += std::to_string(static_cast<long long>(timings[i]));
  }

  return raw_text;
}

template <typename TTiming>
inline std::string raw_timings_to_encoded_text(const std::vector<TTiming> &timings) {
  std::vector<uint8_t> packed;
  packed.reserve(timings.size() * 2U);

  for (TTiming timing_value : timings) {
    int32_t timing = static_cast<int32_t>(timing_value);
    if (timing < static_cast<int32_t>(std::numeric_limits<int16_t>::min())) {
      timing = static_cast<int32_t>(std::numeric_limits<int16_t>::min());
    }
    if (timing > static_cast<int32_t>(std::numeric_limits<int16_t>::max())) {
      timing = static_cast<int32_t>(std::numeric_limits<int16_t>::max());
    }

    const uint16_t encoded = static_cast<uint16_t>(static_cast<int16_t>(timing));
    packed.push_back(static_cast<uint8_t>(encoded & 0xFFU));
    packed.push_back(static_cast<uint8_t>((encoded >> 8) & 0xFFU));
  }

  return esphome::base64_encode(packed);
}

inline bool decode_raw_timings_payload(const std::string &payload, std::vector<int32_t> &out_values) {
  out_values.clear();
  if (!has_text(payload)) {
    return false;
  }

  const std::vector<uint8_t> packed = esphome::base64_decode(payload);
  if (packed.empty() || (packed.size() % 2U) != 0U) {
    return false;
  }

  out_values.reserve(packed.size() / 2U);
  for (size_t i = 0; i < packed.size(); i += 2U) {
    const uint16_t encoded = static_cast<uint16_t>(packed[i]) | (static_cast<uint16_t>(packed[i + 1]) << 8U);
    out_values.push_back(static_cast<int32_t>(static_cast<int16_t>(encoded)));
  }

  return !out_values.empty();
}

inline std::string pending_rf_raw_text() {
  std::vector<int32_t> timings;
  if (!decode_raw_timings_payload(id(rf_pending_raw), timings)) {
    return "";
  }
  return raw_timings_to_text(timings);
}

inline std::string raw_timings_summary_for_ha(const std::vector<int32_t> &timings) {
  if (timings.empty()) {
    return "N:0";
  }

  std::vector<int32_t> abs_values;
  abs_values.reserve(timings.size());

  uint64_t total_us = 0ULL;
  for (size_t i = 0U; i < timings.size(); ++i) {
    const int32_t value = timings[i];
    const int32_t abs_value = (value < 0) ? -value : value;
    if (abs_value <= 0) {
      continue;
    }
    abs_values.push_back(abs_value);
    total_us += static_cast<uint64_t>(abs_value);
  }

  if (abs_values.empty()) {
    return "N:0";
  }

  std::sort(abs_values.begin(), abs_values.end());

  const size_t abs_count = abs_values.size();
  const size_t low_bucket_count = std::max(static_cast<size_t>(3U), abs_count / 3U);
  const size_t low_bucket_index = std::min(low_bucket_count - 1U, abs_count - 1U) / 2U;

  const int32_t pulse_us = abs_values[low_bucket_index];
  const int32_t min_us = abs_values.front();
  const int32_t max_us = abs_values.back();
  const uint32_t avg_us = static_cast<uint32_t>(total_us / static_cast<uint64_t>(abs_count));

  char out[128] = {0};
  std::snprintf(
      out,
      sizeof(out),
      "N:%u P~%dus Avg:%uus Min:%dus Max:%dus T:%lluus",
      static_cast<unsigned int>(timings.size()),
      static_cast<int>(pulse_us),
      static_cast<unsigned int>(avg_us),
      static_cast<int>(min_us),
      static_cast<int>(max_us),
      static_cast<unsigned long long>(total_us));

  return std::string(out);
}

inline std::string button_rf_raw_text_for_ha(const uint8_t button, const char *fallback = kRfCodeNotAssigned) {
  std::vector<int32_t> timings;
  if (!decode_raw_timings_payload(button_rf_raw_text(button), timings)) {
    return std::string(fallback);
  }
  return raw_timings_summary_for_ha(timings);
}

inline std::vector<int32_t> button_rf_raw_timings(const uint8_t button) {
  std::vector<int32_t> timings;
  decode_raw_timings_payload(button_rf_raw_text(button), timings);
  return timings;
}

inline bool parse_raw_timings_from_text(const std::string &text, std::vector<int32_t> &out_values) {
  out_values.clear();
  const char *cursor = text.c_str();

  while (*cursor != '\0') {
    while (*cursor != '\0' && (std::isspace(static_cast<unsigned char>(*cursor)) || *cursor == ',')) {
      cursor++;
    }
    if (*cursor == '\0') {
      break;
    }

    char *end_ptr = nullptr;
    long parsed = std::strtol(cursor, &end_ptr, 10);
    if (end_ptr == cursor) {
      return false;
    }

    if (parsed < static_cast<long>(std::numeric_limits<int16_t>::min())) {
      parsed = static_cast<long>(std::numeric_limits<int16_t>::min());
    }
    if (parsed > static_cast<long>(std::numeric_limits<int16_t>::max())) {
      parsed = static_cast<long>(std::numeric_limits<int16_t>::max());
    }

    out_values.push_back(static_cast<int32_t>(parsed));
    cursor = end_ptr;
  }

  return !out_values.empty();
}

inline void set_button_rf_raw_from_text(const uint8_t button, const std::string &value_in) {
  if (!has_text(value_in) || value_in == kRfCodeNotAssigned) {
    set_button_rf_raw_text(button, "");
    return;
  }

  std::vector<int32_t> timings;
  if (!parse_raw_timings_from_text(value_in, timings)) {
    return;
  }

  set_button_rf_raw_text(button, raw_timings_to_encoded_text(timings));
}

inline size_t button_rf_raw_item_count(const uint8_t button) {
  std::vector<int32_t> timings;
  if (!decode_raw_timings_payload(button_rf_raw_text(button), timings)) {
    return 0U;
  }
  return timings.size();
}

inline size_t pending_rf_raw_item_count() {
  std::vector<int32_t> timings;
  if (!decode_raw_timings_payload(id(rf_pending_raw), timings)) {
    return 0U;
  }
  return timings.size();
}

template <typename TLeft, typename TRight>
inline bool raw_frames_are_similar(const std::vector<TLeft> &left, const std::vector<TRight> &right) {
  if (left.empty() || right.empty()) {
    return false;
  }

  const size_t left_size = left.size();
  const size_t right_size = right.size();
  const size_t size_diff = (left_size > right_size) ? (left_size - right_size) : (right_size - left_size);
  if (size_diff > 4U) {
    return false;
  }

  const size_t compare_count = std::min(std::min(left_size, right_size), static_cast<size_t>(24U));
  if (compare_count < 10U) {
    return false;
  }

  size_t mismatches = 0U;
  for (size_t i = 0U; i < compare_count; ++i) {
    const int32_t a = static_cast<int32_t>(left[i]);
    const int32_t b = static_cast<int32_t>(right[i]);
    const int32_t abs_a = (a < 0) ? -a : a;
    const int32_t abs_b = (b < 0) ? -b : b;
    const int32_t max_v = (abs_a > abs_b) ? abs_a : abs_b;
    if (max_v == 0) {
      continue;
    }
    const int32_t delta = (abs_a > abs_b) ? (abs_a - abs_b) : (abs_b - abs_a);
    if (delta * 100 > max_v * 45) {
      ++mismatches;
    }
  }

  if (mismatches > (compare_count / 4U)) {
    return false;
  }

  int64_t sum_left = 0;
  int64_t sum_right = 0;
  for (size_t i = 0U; i < left_size; ++i) {
    const int64_t v = static_cast<int64_t>(left[i]);
    sum_left += (v < 0) ? -v : v;
  }
  for (size_t i = 0U; i < right_size; ++i) {
    const int64_t v = static_cast<int64_t>(right[i]);
    sum_right += (v < 0) ? -v : v;
  }

  const int64_t max_sum = (sum_left > sum_right) ? sum_left : sum_right;
  const int64_t sum_delta = (sum_left > sum_right) ? (sum_left - sum_right) : (sum_right - sum_left);
  if (max_sum <= 0) {
    return false;
  }

  return (sum_delta * 100) <= (max_sum * 30);
}

inline void enter_rf_learning_mode() {
  id(rf_learning_mode) = true;
  id(rf_pending_raw).clear();
  id(rf_learning_candidate_raw).clear();
  id(rf_last_raw_sample_ms) = 0U;
  wake_backlight();
  set_lcd_message("RF Learn Mode", "Send RF signal", 7000U);
}

inline void leave_rf_learning_mode() {
  id(rf_learning_mode) = false;
  id(rf_pending_raw).clear();
  id(rf_learning_candidate_raw).clear();
  id(rf_last_raw_sample_ms) = 0U;
}

inline void toggle_rf_learning_mode() {
  if (is_rf_learning_mode()) {
    leave_rf_learning_mode();
    wake_backlight();
    clear_lcd_message();
  } else {
    enter_rf_learning_mode();
  }
}

template <typename TTiming>
inline bool on_rf_raw_received(const std::vector<TTiming> &timings) {
  if (!is_rf_learning_mode()) {
    return false;
  }

  // Ignore obvious noise bursts and malformed captures.
  if (timings.size() < 20U || timings.size() > 220U) {
    return false;
  }

  const uint32_t now = now_ms();
  // Raw callbacks can be very frequent; keep learning responsive but bounded.
  if (id(rf_last_raw_sample_ms) != 0U && static_cast<uint32_t>(now - id(rf_last_raw_sample_ms)) < 120U) {
    return false;
  }

  const std::string encoded = raw_timings_to_encoded_text(timings);
  if (!has_text(id(rf_learning_candidate_raw))) {
    id(rf_learning_candidate_raw) = encoded;
    id(rf_last_raw_sample_ms) = now;
    return false;
  }

  if (encoded == id(rf_learning_candidate_raw)) {
    id(rf_pending_raw) = encoded;
    id(rf_learning_candidate_raw).clear();
    id(rf_last_raw_sample_ms) = now;
  } else {
    std::vector<int32_t> candidate_timings;
    if (decode_raw_timings_payload(id(rf_learning_candidate_raw), candidate_timings) &&
        raw_frames_are_similar(candidate_timings, timings)) {
      id(rf_pending_raw) = encoded;
      id(rf_learning_candidate_raw).clear();
      id(rf_last_raw_sample_ms) = now;
    } else {
      id(rf_learning_candidate_raw) = encoded;
      id(rf_last_raw_sample_ms) = now;
      return false;
    }
  }

  wake_backlight();

  char line_2[17] = {0};
  std::snprintf(line_2, sizeof(line_2), "%u raw items", static_cast<unsigned int>(timings.size()));
  set_lcd_message("RAW Captured", line_2, 8000U);
  return true;
}

inline bool assign_pending_rf_to_button(const uint8_t button) {
  if (!is_rf_learning_mode()) {
    return false;
  }

  if (!pending_rf_raw_enabled()) {
    wake_backlight();
    set_lcd_message("No RAW Capt", "Send RF signal", 5000U);
    return false;
  }

  set_button_rf_raw_text(button, id(rf_pending_raw));

  char line_1[17] = {0};
  char line_2[17] = {0};
  std::snprintf(line_1, sizeof(line_1), "Saved to Btn %u", button);
  std::snprintf(line_2, sizeof(line_2), "%u raw items", static_cast<unsigned int>(button_rf_raw_item_count(button)));
  wake_backlight();
  set_lcd_message(line_1, line_2, 5000U);
  leave_rf_learning_mode();
  return true;
}

inline void show_assigned_rf_for_button(const uint8_t button) {
  if (!button_rf_enabled(button)) {
    return;
  }

  char line_1[17] = {0};
  char line_2[17] = {0};
  std::snprintf(line_1, sizeof(line_1), "Btn %u RAW Sent", button);
  std::snprintf(line_2, sizeof(line_2), "%u raw items", static_cast<unsigned int>(button_rf_raw_item_count(button)));

  wake_backlight();
  set_lcd_message(line_1, line_2, 4000U);
}

inline void show_rf_test_result(const uint8_t button, const bool has_raw) {
  char line_1[17] = {0};
  char line_2[17] = {0};

  std::snprintf(line_1, sizeof(line_1), "RF Test Btn %u", button);
  if (!has_raw) {
    std::snprintf(line_2, sizeof(line_2), "No RAW assigned");
    wake_backlight();
    set_lcd_message(line_1, line_2, 5000U);
    return;
  }

  std::snprintf(line_2, sizeof(line_2), "%u raw items", static_cast<unsigned int>(button_rf_raw_item_count(button)));
  wake_backlight();
  set_lcd_message(line_1, line_2, 5000U);
}

inline void note_user_action() {
  wake_backlight();
}

inline float clamp_servo_angle(const float &angle_deg, const float &max_angle_deg) {
  if (!std::isfinite(angle_deg)) {
    return 0.0f;
  }
  if (!std::isfinite(max_angle_deg) || max_angle_deg <= 0.0f) {
    return 0.0f;
  }
  if (angle_deg < 0.0f) {
    return 0.0f;
  }
  if (angle_deg > max_angle_deg) {
    return max_angle_deg;
  }
  return angle_deg;
}

inline float angle_to_servo_level(const float &angle_deg, const float &max_angle_deg) {
  const float clamped = clamp_servo_angle(angle_deg, max_angle_deg);
  if (!std::isfinite(max_angle_deg) || max_angle_deg <= 0.0f) {
    return -1.0f;
  }
  return (clamped / max_angle_deg) * 2.0f - 1.0f;
}

inline bool is_servo_near_start(const float &angle_deg, const float &start_deg, const float &stop_deg) {
  const float d_start = std::fabs(angle_deg - start_deg);
  const float d_stop = std::fabs(angle_deg - stop_deg);
  return d_start <= d_stop;
}

inline float interpolate_servo_angle(const float &from_deg, const float &to_deg, const float &ratio) {
  if (!std::isfinite(from_deg) || !std::isfinite(to_deg) || !std::isfinite(ratio)) {
    return from_deg;
  }
  float t = ratio;
  if (t < 0.0f) {
    t = 0.0f;
  }
  if (t > 1.0f) {
    t = 1.0f;
  }
  return from_deg + (to_deg - from_deg) * t;
}

inline float left_range_min_deg() {
  return std::fmin(id(servo_1_start_deg), id(servo_1_stop_deg));
}

inline float left_range_max_deg() {
  return std::fmax(id(servo_1_start_deg), id(servo_1_stop_deg));
}

inline float right_range_min_deg() {
  return std::fmin(id(servo_2_start_deg), id(servo_2_stop_deg));
}

inline float right_range_max_deg() {
  return std::fmax(id(servo_2_start_deg), id(servo_2_stop_deg));
}

inline float clamp_to_range(const float &value, const float &min_value, const float &max_value) {
  if (!std::isfinite(value)) {
    return min_value;
  }
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return value;
}

inline float clamp_left_servo_to_saved_range(const float &angle_deg, const float &max_angle_deg) {
  const float clamped = clamp_servo_angle(angle_deg, max_angle_deg);
  return clamp_to_range(clamped, left_range_min_deg(), left_range_max_deg());
}

inline float clamp_right_servo_to_saved_range(const float &angle_deg, const float &max_angle_deg) {
  const float clamped = clamp_servo_angle(angle_deg, max_angle_deg);
  return clamp_to_range(clamped, right_range_min_deg(), right_range_max_deg());
}

inline void move_left_servo_to_angle(const float &target_deg, const float &max_angle_deg) {
  const float clamped = clamp_left_servo_to_saved_range(target_deg, max_angle_deg);
  id(servo_1_current_deg) = clamped;
  id(servo_1_near_start) = is_servo_near_start(clamped, id(servo_1_start_deg), id(servo_1_stop_deg));
}

inline void move_right_servo_to_angle(const float &target_deg, const float &max_angle_deg) {
  const float clamped = clamp_right_servo_to_saved_range(target_deg, max_angle_deg);
  id(servo_2_current_deg) = clamped;
  id(servo_2_near_start) = is_servo_near_start(clamped, id(servo_2_start_deg), id(servo_2_stop_deg));
}

inline void save_target_angles_as_stop(const float &max_angle_deg) {
  id(servo_1_stop_deg) = clamp_servo_angle(id(servo_1_target_deg_input_value), max_angle_deg);
  id(servo_2_stop_deg) = clamp_servo_angle(id(servo_2_target_deg_input_value), max_angle_deg);
}

inline void save_target_angles_as_start(const float &max_angle_deg) {
  id(servo_1_start_deg) = clamp_servo_angle(id(servo_1_target_deg_input_value), max_angle_deg);
  id(servo_2_start_deg) = clamp_servo_angle(id(servo_2_target_deg_input_value), max_angle_deg);
}

inline void step_servos_toward_stop(const float &ratio, const float &max_angle_deg) {
  id(servo_1_current_deg) = clamp_left_servo_to_saved_range(
      interpolate_servo_angle(id(servo_1_current_deg), id(servo_1_stop_deg), ratio),
      max_angle_deg);
  id(servo_2_current_deg) = clamp_right_servo_to_saved_range(
      interpolate_servo_angle(id(servo_2_current_deg), id(servo_2_stop_deg), ratio),
      max_angle_deg);
}

inline void reset_motor_hours_counter() {
  id(motor_hours_total) = 0.0f;
}

inline float motor_hours_offset_value() {
  return id(motor_hours_offset);
}

inline void set_motor_hours_offset(const float &hours) {
  if (!std::isfinite(hours) || hours < 0.0f) {
    return;
  }
  id(motor_hours_offset) = hours;
}

inline void reset_all_counters() {
  reset_motor_hours_counter();
}

inline void mark_pzem_rx_if_finite(const float &value) {
  if (std::isfinite(value)) {
    id(pzem_last_rx_ms) = now_ms();
  }
}

inline bool is_pzem_data_fresh(const uint32_t max_age_ms = 5000U) {
  if (id(pzem_last_rx_ms) == 0U) {
    return false;
  }
  return static_cast<uint32_t>(now_ms() - id(pzem_last_rx_ms)) <= max_age_ms;
}

inline bool is_generator_running_with_pzem_freshness(
    const float &voltage,
    const float &threshold,
    const uint32_t max_age_ms = 5000U) {
  if (!is_pzem_data_fresh(max_age_ms)) {
    return false;
  }
  return is_generator_running(voltage, threshold);
}

template <typename TSensor>
inline void publish_sensor_zero_if_needed(const float &state, TSensor &sensor) {
  if (!std::isfinite(state) || state != 0.0f) {
    sensor.publish_state(0.0f);
  }
}

inline void zero_pzem_sensors_if_stale(const uint32_t max_age_ms = 5000U) {
  if (is_pzem_data_fresh(max_age_ms)) {
    return;
  }

  publish_sensor_zero_if_needed(id(pzem_voltage).state, id(pzem_voltage));
  publish_sensor_zero_if_needed(id(pzem_current).state, id(pzem_current));
  publish_sensor_zero_if_needed(id(pzem_power).state, id(pzem_power));
  publish_sensor_zero_if_needed(id(pzem_frequency).state, id(pzem_frequency));
  publish_sensor_zero_if_needed(id(pzem_power_factor).state, id(pzem_power_factor));
}

inline void on_button_long_action(const uint8_t button) {
  note_user_action();
  assign_pending_rf_to_button(button);
}

inline float power_for_ha_calc(const float &power) {
  return std::isfinite(power) ? power : 0.0f;
}

inline float motor_hours_total_value() {
  return id(motor_hours_total) + id(motor_hours_offset);
}

inline void accumulate_motor_hours(const bool &running, const float &dt_seconds = 1.0f) {
  if (!running || !std::isfinite(dt_seconds) || dt_seconds <= 0.0f) {
    return;
  }
  id(motor_hours_total) += dt_seconds / 3600.0f;
}

inline void accumulate_runtime_tick(const bool &running) {
  static uint32_t last_tick_ms = 0U;

  const uint32_t now = now_ms();
  if (last_tick_ms == 0U) {
    last_tick_ms = now;
    return;
  }

  uint32_t dt_ms = static_cast<uint32_t>(now - last_tick_ms);
  last_tick_ms = now;
  if (dt_ms == 0U) {
    return;
  }

  // Cap long scheduler gaps so a paused loop does not over-add counters.
  constexpr uint32_t kMaxDtMs = 5000U;
  if (dt_ms > kMaxDtMs) {
    dt_ms = kMaxDtMs;
  }

  const float dt_seconds = static_cast<float>(dt_ms) / 1000.0f;
  accumulate_motor_hours(running, dt_seconds);
}

inline const std::string &last_run_start_text() {
  return id(last_run_start_text_value);
}

inline const std::string &last_run_stop_text() {
  return id(last_run_stop_text_value);
}

inline const std::string &last_run_duration_text() {
  return id(last_run_duration);
}

inline std::string format_duration_hms(const uint32_t total_seconds) {
  const uint32_t hours = total_seconds / 3600U;
  const uint32_t minutes = (total_seconds % 3600U) / 60U;
  const uint32_t seconds = total_seconds % 60U;

  char buffer[16] = {0};
  std::snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu", static_cast<unsigned long>(hours),
                static_cast<unsigned long>(minutes), static_cast<unsigned long>(seconds));
  return std::string(buffer);
}

inline std::string format_duration_hm(const uint32_t total_minutes) {
  const uint32_t hours = total_minutes / 60U;
  const uint32_t minutes = total_minutes % 60U;

  char buffer[16] = {0};
  std::snprintf(buffer, sizeof(buffer), "%02lu:%02lu", static_cast<unsigned long>(hours),
                static_cast<unsigned long>(minutes));
  return std::string(buffer);
}

template <typename TNow>
inline std::string format_time_or_unknown(TNow now) {
  if (now.is_valid()) {
    return now.strftime("%Y-%m-%d %H:%M:%S");
  }
  return "unknown";
}

template <typename TNow>
inline void on_generator_run_started(TNow now) {
  const int32_t started = id(last_run_start_unix_s);
  if (started > 0) {
    if (!now.is_valid() || now.timestamp >= started) {
      if (!has_text(id(last_run_start_text_value)) && now.is_valid()) {
        id(last_run_start_text_value) = now.strftime("%Y-%m-%d %H:%M:%S");
      }
      return;
    }
  }

  id(last_run_start_text_value) = format_time_or_unknown(now);

  if (now.is_valid()) {
    id(last_run_start_unix_s) = now.timestamp;
  } else {
    id(last_run_start_unix_s) = 0;
  }
}

template <typename TNow>
inline void on_generator_run_stopped(TNow now) {
  id(last_run_stop_text_value) = format_time_or_unknown(now);

  const int32_t started = id(last_run_start_unix_s);
  if (!now.is_valid() || started <= 0 || now.timestamp < started) {
    id(last_run_duration) = "unknown";
    id(last_run_start_unix_s) = 0;
    return;
  }

  const uint32_t elapsed_minutes = static_cast<uint32_t>(now.timestamp - started) / 60U;
  id(last_run_duration) = format_duration_hm(elapsed_minutes);
  id(last_run_start_unix_s) = 0;
}

template <typename TNow>
inline std::string current_run_duration_text(TNow now, const bool &running) {
  if (!running) {
    return "00:00";
  }

  if (!now.is_valid()) {
    return "unknown";
  }

  const int32_t started = id(last_run_start_unix_s);
  if (started <= 0 || now.timestamp < started) {
    // After restart while already running, initialize from current time.
    id(last_run_start_unix_s) = now.timestamp;
    id(last_run_start_text_value) = now.strftime("%Y-%m-%d %H:%M:%S");
    return "00:00";
  }

  const uint32_t elapsed_minutes = static_cast<uint32_t>(now.timestamp - started) / 60U;
  return format_duration_hm(elapsed_minutes);
}

template <typename TDisplay>
inline void render_status_page(
    TDisplay &display,
    const bool &display_enabled,
    const bool &eco_enabled,
    const bool &rf_enabled,
    const bool &generator_running,
    const float &voltage,
    const float &power) {
  if (!display_enabled) {
    display.no_backlight();
    display.clear();
    return;
  }

  display.backlight();

  char line_1[17] = {0};
  char line_2[17] = {0};

  const float safe_voltage = std::isfinite(voltage) ? voltage : 0.0f;
  const float safe_power = std::isfinite(power) ? power : 0.0f;

  std::snprintf(line_1, sizeof(line_1), "V:%5.1f P:%4.0fW", safe_voltage, safe_power);
  std::snprintf(
      line_2,
      sizeof(line_2),
      "E:%s R:%s G:%s",
      on_off_text(eco_enabled),
      on_off_text(rf_enabled),
      on_off_text(generator_running));

  display.print(0, 0, line_1);
  display.print(0, 1, line_2);
}

template <typename TDisplay>
inline void render_environment_page(
    TDisplay &display,
    const bool &display_enabled,
    const float &temperature,
    const float &humidity) {
  if (!display_enabled) {
    return;
  }

  char line_1[17] = {0};
  char line_2[17] = {0};

  const float safe_temperature = std::isfinite(temperature) ? temperature : 0.0f;
  const float safe_humidity = std::isfinite(humidity) ? humidity : 0.0f;

  std::snprintf(line_1, sizeof(line_1), "Temp:%5.1f C", safe_temperature);
  std::snprintf(line_2, sizeof(line_2), "Hum :%5.1f %%", safe_humidity);

  display.print(0, 0, line_1);
  display.print(0, 1, line_2);
}

template <typename TDisplay>
inline void render_lcd_page(
    TDisplay &display,
    const bool &display_enabled,
    const float &temperature,
    const float &humidity,
    const float &voltage,
    const float &current,
    const float &power,
    const float &power_factor) {
  if (!display_enabled) {
    display.no_backlight();
    display.clear();
    return;
  }

  if (is_backlight_active()) {
    display.backlight();
  } else {
    display.no_backlight();
  }

  char line_1[17] = {0};
  char line_2[17] = {0};

  const float safe_temperature = std::isfinite(temperature) ? temperature : 0.0f;
  const float safe_humidity = std::isfinite(humidity) ? humidity : 0.0f;
  const float safe_voltage = std::isfinite(voltage) ? voltage : 0.0f;
  const float safe_current = std::isfinite(current) ? current : 0.0f;
  const float safe_power = std::isfinite(power) ? power : 0.0f;
  const float safe_power_factor = std::isfinite(power_factor) ? power_factor : 0.0f;

  std::snprintf(line_1, sizeof(line_1), "T:%4.1fC H:%4.1f%%", safe_temperature, safe_humidity);
  if (id(lcd_line2_mode) == 0) {
    std::snprintf(line_2, sizeof(line_2), "V:%5.1f P:%4.0fW", safe_voltage, safe_power);
  } else {
    std::snprintf(line_2, sizeof(line_2), "I:%4.2fA  PF:%1.2f", safe_current, safe_power_factor);
  }

  if (is_lcd_message_active()) {
    copy_16(line_1, id(lcd_message_line_1));
    copy_16(line_2, id(lcd_message_line_2));
  } else if (is_rf_learning_mode()) {
    std::snprintf(line_1, sizeof(line_1), "RF Learn Mode");
    if (pending_rf_raw_enabled()) {
      std::snprintf(line_2, sizeof(line_2), "%u raw items", static_cast<unsigned int>(pending_rf_raw_item_count()));
    } else {
      std::snprintf(line_2, sizeof(line_2), "Send RF signal");
    }
  } else if (is_ap_mode_active()) {
    char ap_ssid[17] = {0};
    active_ap_ssid_or_default(ap_ssid);
    std::snprintf(line_2, sizeof(line_2), "AP:%s", ap_ssid);
  }

  display.print(0, 0, line_1);
  display.print(0, 1, line_2);
}

}  // namespace garage
