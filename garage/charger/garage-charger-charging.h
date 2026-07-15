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
    FontType *font_big,
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

  // Panel is a genuine 128x64 bicolor SSD1306: top ~16px yellow, rest blue.
  constexpr int kScreenW = 128;
  constexpr int kScreenH = 64;
  constexpr int kYellowZoneH = 16;

  if (captive_portal_active) {
    it.printf(0, 0, font, "AP MODE");
    // Wrap the AP name across two lines - full panel width holds roughly 18
    // characters per line at this font size.
    char line1[19];
    std::snprintf(line1, sizeof(line1), "%s", ap_name != nullptr ? ap_name : "");
    it.printf(0, 16, font, "%s", line1);
    const size_t shown = std::strlen(line1);
    if (ap_name != nullptr && std::strlen(ap_name) > shown) {
      it.printf(0, 32, font, "%s", ap_name + shown);
    }
    // Bottom: current battery voltage, still useful to see while stuck in
    // AP/setup mode. Mid-sized font (same as the AP MODE/AP name text above).
    if (voltage_valid && std::isfinite(voltage)) {
      it.printf(kScreenW / 2, kScreenH, font, esphome::display::TextAlign::BOTTOM_CENTER, "%.1fV", voltage);
    } else {
      it.printf(kScreenW / 2, kScreenH, font, esphome::display::TextAlign::BOTTOM_CENTER, "V:n/a");
    }
    return;
  }

  // Top-left (yellow zone): ON/OFF status, unchanged.
  it.printf(0, 0, font, "%s", charging ? "ON" : "OFF");

  // Top-right (yellow zone): charge-start/charge-stop voltage thresholds on
  // a single line (stacking two lines here kept overlapping/garbling - this
  // panel's actual small-font glyph height is larger than expected and hard
  // to pin down without hardware in the loop), flush against the right edge
  // regardless of digit count.
  it.printf(
      kScreenW, 0, font_small, esphome::display::TextAlign::TOP_RIGHT, "L:%.1f H:%.1f", low_voltage, high_voltage);

  // Center of the blue zone: the current battery voltage, as big as this
  // panel reasonably fits, since it's the single most important glanceable
  // value.
  const int blue_center_y = kYellowZoneH + (kScreenH - kYellowZoneH) / 2;
  if (voltage_valid && std::isfinite(voltage)) {
    it.printf(kScreenW / 2, blue_center_y, font_big, esphome::display::TextAlign::CENTER, "%.1fV", voltage);
  } else {
    it.printf(kScreenW / 2, blue_center_y, font_big, esphome::display::TextAlign::CENTER, "n/a");
  }

  // Bottom corners (blue zone): temperature (left) and humidity (right).
  if (temp_valid && std::isfinite(temperature_c)) {
    it.printf(0, kScreenH, font_small, esphome::display::TextAlign::BOTTOM_LEFT, "%.0fC", temperature_c);
  } else {
    it.printf(0, kScreenH, font_small, esphome::display::TextAlign::BOTTOM_LEFT, "n/a");
  }
  if (humidity_valid && std::isfinite(humidity_percent)) {
    it.printf(
        kScreenW, kScreenH, font_small, esphome::display::TextAlign::BOTTOM_RIGHT, "%.0f%%", humidity_percent);
  } else {
    it.printf(kScreenW, kScreenH, font_small, esphome::display::TextAlign::BOTTOM_RIGHT, "n/a");
  }
}

}  // namespace garage_charger
