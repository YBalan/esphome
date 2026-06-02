#pragma once

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>

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

inline void clear_lcd_message() {
  id(lcd_message_line_1).clear();
  id(lcd_message_line_2).clear();
  id(lcd_message_until_ms) = 0U;
}

inline bool is_lcd_message_active() {
  return time_until_in_future(id(lcd_message_until_ms));
}

inline std::string clip_16(const std::string &value) {
  if (value.size() <= 16U) {
    return value;
  }
  return value.substr(0, 16);
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

inline int button_rf_protocol(const uint8_t button) {
  switch (button) {
    case 1:
      return id(rf_protocol_btn_1);
    case 2:
      return id(rf_protocol_btn_2);
    case 3:
      return id(rf_protocol_btn_3);
    case 4:
      return id(rf_protocol_btn_4);
    default:
      return 1;
  }
}

inline void set_button_rf_data(const uint8_t button, const std::string &code, const int &protocol) {
  switch (button) {
    case 1:
      id(rf_code_btn_1) = code;
      id(rf_protocol_btn_1) = protocol;
      break;
    case 2:
      id(rf_code_btn_2) = code;
      id(rf_protocol_btn_2) = protocol;
      break;
    case 3:
      id(rf_code_btn_3) = code;
      id(rf_protocol_btn_3) = protocol;
      break;
    case 4:
      id(rf_code_btn_4) = code;
      id(rf_protocol_btn_4) = protocol;
      break;
    default:
      break;
  }
}

// True when the matching learned RF code is present and can be transmitted.
inline bool button_rf_enabled(const uint8_t button) {
  return has_text(button_rf_code(button));
}

inline bool is_rf_learning_mode() {
  return id(rf_learning_mode);
}

inline void enter_rf_learning_mode() {
  id(rf_learning_mode) = true;
  id(rf_pending_code).clear();
  id(rf_pending_protocol) = 1;
  wake_backlight();
  set_lcd_message("RF Learn Mode", "Send RF signal", 7000U);
}

inline void leave_rf_learning_mode() {
  id(rf_learning_mode) = false;
  id(rf_pending_code).clear();
  id(rf_pending_protocol) = 1;
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

inline bool on_rf_signal_received(const uint64_t &code, const uint8_t &protocol) {
  if (!is_rf_learning_mode()) {
    return false;
  }

  id(rf_pending_code) = u64_to_binary(code);
  id(rf_pending_protocol) = static_cast<int>(protocol);
  wake_backlight();

  const std::string line_2 = clip_16(id(rf_pending_code));
  set_lcd_message("RF Captured", line_2, 8000U);
  return true;
}

inline bool assign_pending_rf_to_button(const uint8_t button) {
  if (!is_rf_learning_mode()) {
    return false;
  }

  if (!has_text(id(rf_pending_code))) {
    wake_backlight();
    set_lcd_message("No RF Captured", "Send RF signal", 5000U);
    return false;
  }

  set_button_rf_data(button, id(rf_pending_code), id(rf_pending_protocol));

  char line_1[17] = {0};
  std::snprintf(line_1, sizeof(line_1), "Saved to Btn %u", button);
  wake_backlight();
  set_lcd_message(line_1, "RF assigned", 5000U);
  leave_rf_learning_mode();
  return true;
}

inline void show_assigned_rf_for_button(const uint8_t button) {
  if (!button_rf_enabled(button)) {
    return;
  }

  char line_1[17] = {0};
  std::snprintf(line_1, sizeof(line_1), "Btn %u RF", button);

  const std::string code = clip_16(button_rf_code(button));
  wake_backlight();
  set_lcd_message(line_1, code, 4000U);
}

inline void note_user_action() {
  wake_backlight();
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

inline void reset_total_energy_counter() {
  id(total_energy_kwh) = 0.0f;
}

inline void reset_all_counters() {
  reset_motor_hours_counter();
  reset_total_energy_counter();
}

inline void set_button_rf_code_from_text(const uint8_t button, const std::string &value_in) {
  std::string value = value_in;
  if (value == kRfCodeNotAssigned) {
    value.clear();
  }
  set_button_rf_data(button, value, button_rf_protocol(button));
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

inline float total_energy_counter_kwh() {
  return id(total_energy_kwh);
}

inline float motor_hours_total_value() {
  return id(motor_hours_total) + id(motor_hours_offset);
}

inline void accumulate_total_energy_kwh(const float &power_watts, const float &dt_seconds = 1.0f) {
  if (!std::isfinite(power_watts) || !std::isfinite(dt_seconds) || power_watts <= 0.0f || dt_seconds <= 0.0f) {
    return;
  }
  id(total_energy_kwh) += (power_watts * dt_seconds) / 3600000.0f;
}

inline void accumulate_motor_hours(const bool &running, const float &dt_seconds = 1.0f) {
  if (!running || !std::isfinite(dt_seconds) || dt_seconds <= 0.0f) {
    return;
  }
  id(motor_hours_total) += dt_seconds / 3600.0f;
}

inline const std::string &last_run_timestamp_text() {
  return id(last_run_timestamp);
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

template <typename TNow>
inline void update_last_run_timestamp(TNow now) {
  if (now.is_valid()) {
    id(last_run_timestamp) = now.strftime("%Y-%m-%d %H:%M:%S");
  } else {
    id(last_run_timestamp) = "unknown";
  }
}

template <typename TNow>
inline void on_generator_run_started(TNow now) {
  if (now.is_valid()) {
    id(last_run_start_unix_s) = now.timestamp;
  } else {
    id(last_run_start_unix_s) = 0;
  }
}

template <typename TNow>
inline void on_generator_run_stopped(TNow now) {
  update_last_run_timestamp(now);

  const int32_t started = id(last_run_start_unix_s);
  if (!now.is_valid() || started <= 0 || now.timestamp < started) {
    id(last_run_duration) = "unknown";
    id(last_run_start_unix_s) = 0;
    return;
  }

  const uint32_t duration_seconds = static_cast<uint32_t>(now.timestamp - started);
  id(last_run_duration) = format_duration_hms(duration_seconds);
  id(last_run_start_unix_s) = 0;
}

template <typename TNow>
inline std::string current_run_duration_text(TNow now, const bool &running) {
  if (!running) {
    return "00:00:00";
  }

  const int32_t started = id(last_run_start_unix_s);
  if (!now.is_valid() || started <= 0 || now.timestamp < started) {
    return "unknown";
  }

  const uint32_t duration_seconds = static_cast<uint32_t>(now.timestamp - started);
  return format_duration_hms(duration_seconds);
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
  static uint32_t line2_last_switch_ms = 0U;
  static bool line2_show_voltage_power = true;
  constexpr uint32_t line2_switch_period_ms = 3000U;
  const uint32_t now = now_ms();

  if (line2_last_switch_ms == 0U) {
    line2_last_switch_ms = now;
  } else if (static_cast<uint32_t>(now - line2_last_switch_ms) >= line2_switch_period_ms) {
    line2_last_switch_ms = now;
    line2_show_voltage_power = !line2_show_voltage_power;
  }

  if (line2_show_voltage_power) {
    std::snprintf(line_2, sizeof(line_2), "V:%5.1f P:%4.0fW", safe_voltage, safe_power);
  } else {
    std::snprintf(line_2, sizeof(line_2), "I:%4.2fA  PF:%1.2f", safe_current, safe_power_factor);
  }

  if (is_lcd_message_active()) {
    const std::string msg_1 = clip_16(id(lcd_message_line_1));
    const std::string msg_2 = clip_16(id(lcd_message_line_2));
    std::snprintf(line_1, sizeof(line_1), "%s", msg_1.c_str());
    std::snprintf(line_2, sizeof(line_2), "%s", msg_2.c_str());
  } else if (is_rf_learning_mode()) {
    std::snprintf(line_1, sizeof(line_1), "RF Learn Mode");
    if (has_text(id(rf_pending_code))) {
      const std::string learn_code = clip_16(id(rf_pending_code));
      std::snprintf(line_2, sizeof(line_2), "%s", learn_code.c_str());
    } else {
      std::snprintf(line_2, sizeof(line_2), "Send RF signal");
    }
  }

  display.print(0, 0, line_1);
  display.print(0, 1, line_2);
}

}  // namespace garage
