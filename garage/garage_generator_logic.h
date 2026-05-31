#pragma once

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>

namespace garage {

inline bool is_generator_running(const float &voltage, const float &threshold) {
  if (!std::isfinite(voltage)) {
    return false;
  }
  return voltage >= threshold;
}

inline const char *on_off_text(const bool &value) {
  return value ? "ON" : "OFF";
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

// True when the matching learned RF code is present and can be transmitted.
inline bool button_rf_enabled(const uint8_t button) {
  return has_text(button_rf_code(button));
}

inline float power_for_ha_calc(const float &power) {
  return std::isfinite(power) ? power : 0.0f;
}

inline float total_energy_counter_kwh() {
  return id(total_energy_kwh);
}

inline float motor_hours_total_value() {
  return id(motor_hours_total);
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

template <typename TNow>
inline void update_last_run_timestamp(TNow now) {
  if (now.is_valid()) {
    id(last_run_timestamp) = now.strftime("%Y-%m-%d %H:%M:%S");
  } else {
    id(last_run_timestamp) = "unknown";
  }
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

  std::snprintf(line_1, sizeof(line_1), "V:%5.1f P:%4.0f", safe_voltage, safe_power);
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

  display.backlight();

  char line_1[17] = {0};
  char line_2[17] = {0};

  const float safe_temperature = std::isfinite(temperature) ? temperature : 0.0f;
  const float safe_humidity = std::isfinite(humidity) ? humidity : 0.0f;
  const float safe_voltage = std::isfinite(voltage) ? voltage : 0.0f;
  const float safe_current = std::isfinite(current) ? current : 0.0f;
  const float safe_power = std::isfinite(power) ? power : 0.0f;
  const float safe_power_factor = std::isfinite(power_factor) ? power_factor : 0.0f;

  std::snprintf(line_1, sizeof(line_1), "T:%4.1fC H:%4.1f%%", safe_temperature, safe_humidity);

  static uint8_t pzem_line_page = 0;
  if (pzem_line_page == 0) {
    std::snprintf(line_2, sizeof(line_2), "V:%5.1f I:%3.1f", safe_voltage, safe_current);
  } else {
    std::snprintf(line_2, sizeof(line_2), "P:%4.0f PF:%1.2f", safe_power, safe_power_factor);
  }
  pzem_line_page = static_cast<uint8_t>((pzem_line_page + 1U) % 2U);

  display.print(0, 0, line_1);
  display.print(0, 1, line_2);
}

}  // namespace garage
