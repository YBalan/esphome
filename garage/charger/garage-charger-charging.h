#pragma once

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace garage_charger {

// Hysteresis charging decision, gated by a safe temperature band. Returns
// false (never charge) whenever voltage/temperature are unavailable or the
// temperature is outside [temp_low, temp_high].
inline bool decide_charging(
    const bool currently_charging,
    const bool voltage_valid,
    const float voltage,
    const bool temp_valid,
    const float temperature_c,
    const float low_voltage,
    const float high_voltage,
    const float temp_low,
    const float temp_high) {
  if (!voltage_valid || !std::isfinite(voltage) || !temp_valid || !std::isfinite(temperature_c)) {
    return false;
  }
  if (temperature_c < temp_low || temperature_c > temp_high) {
    return false;
  }
  if (currently_charging) {
    return voltage < high_voltage;
  }
  return voltage <= low_voltage;
}

// Approximate battery SOC (%), linearly mapped between the two charge
// thresholds: low_voltage (Start Voltage) -> 0%, high_voltage (Stop
// Voltage) -> 100%, clamped to that range. Returns NAN when the voltage is
// unavailable or the thresholds are misconfigured (high <= low).
inline float battery_percent(
    const bool voltage_valid,
    const float voltage,
    const float low_voltage,
    const float high_voltage) {
  if (!voltage_valid || !std::isfinite(voltage) || high_voltage <= low_voltage) {
    return NAN;
  }
  float pct = (voltage - low_voltage) / (high_voltage - low_voltage) * 100.0f;
  if (pct < 0.0f) {
    pct = 0.0f;
  } else if (pct > 100.0f) {
    pct = 100.0f;
  }
  return pct;
}

// Formats a duration as zero-padded "HH:mm" (hours may exceed 2 digits for
// very long durations; minutes are always 2 digits).
inline void format_duration_hhmm(const uint32_t total_seconds, char *buf, const size_t buf_len) {
  const uint32_t hours = total_seconds / 3600;
  const uint32_t minutes = (total_seconds % 3600) / 60;
  std::snprintf(buf, buf_len, "%02u:%02u", (unsigned) hours, (unsigned) minutes);
}

template<typename DisplayType, typename FontType>
inline void render_display(
    DisplayType &it,
    FontType *font,
    FontType *font_small,
    const bool captive_portal_active,
    const char *ap_name,
    const bool charging,
    const bool temp_valid,
    const float temperature_c,
    const bool humidity_valid,
    const float humidity_percent,
    const bool voltage_valid,
    const float voltage,
    const float low_voltage,
    const float high_voltage) {
  // Explicit clear (in addition to auto_clear_enabled) guards against stale
  // GDDRAM pixels left over from a prior firmware's boot-time content that
  // this frame doesn't happen to overwrite.
  it.clear();

  if (captive_portal_active) {
    it.printf(0, 0, font, "AP MODE");
    // Wrap the AP name across two lines - this panel is only ~64px wide, so
    // each line holds roughly 9 characters at this font size.
    char line1[10];
    std::snprintf(line1, sizeof(line1), "%s", ap_name != nullptr ? ap_name : "");
    it.printf(0, 16, font, "%s", line1);
    const size_t shown = std::strlen(line1);
    if (ap_name != nullptr && std::strlen(ap_name) > shown) {
      it.printf(0, 32, font, "%s", ap_name + shown);
    }
    return;
  }

  // Row 0: big ON/OFF status, with the voltage reading in a smaller font
  // right after it on the same line.
  it.printf(0, 0, font, "%s", charging ? "ON" : "OFF");
  if (voltage_valid && std::isfinite(voltage)) {
    it.printf(34, 3, font_small, "%.1fV", voltage);
  } else {
    it.printf(34, 3, font_small, "n/a");
  }

  // Row 1: temperature and humidity together.
  char temp_buf[6];
  if (temp_valid && std::isfinite(temperature_c)) {
    std::snprintf(temp_buf, sizeof(temp_buf), "%.0fC", temperature_c);
  } else {
    std::snprintf(temp_buf, sizeof(temp_buf), "n/a");
  }
  char hum_buf[6];
  if (humidity_valid && std::isfinite(humidity_percent)) {
    std::snprintf(hum_buf, sizeof(hum_buf), "%.0f%%", humidity_percent);
  } else {
    std::snprintf(hum_buf, sizeof(hum_buf), "n/a");
  }
  it.printf(0, 16, font_small, "T:%s H:%s", temp_buf, hum_buf);

  it.printf(0, 25, font_small, "Lo:%.2f", low_voltage);
  it.printf(0, 34, font_small, "Hi:%.2f", high_voltage);
}

}  // namespace garage_charger
