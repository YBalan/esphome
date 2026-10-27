#pragma once

namespace jbdbms_lambdas {

inline float calculate_charging_time() {
  if (!id(bms_total_capacity).has_state() || !id(bms_a).has_state() || !id(bms_ah).has_state()) {
    return 0;
  }

  float current = id(bms_a).state;
  float total_cap = id(bms_total_capacity).state;
  float remaining_cap = id(bms_ah).state;

  if (current > 0.1f) {
    float need_ah = total_cap - remaining_cap;
    if (need_ah <= 0) {
      return 0;
    }
    return need_ah / current;
  }

  return 0;
}

inline float calculate_soc_percentage() {
  if (!id(bms_total_capacity).has_state() || !id(bms_ah).has_state()) {
    return NAN;
  }

  float total_capacity = id(bms_total_capacity).state;
  if (total_capacity <= 0.0f) {
    return NAN;
  }

  float soc_percent = (id(bms_ah).state / total_capacity) * 100.0f;
  if (soc_percent < 0.0f) {
    soc_percent = 0.0f;
  } else if (soc_percent > 100.0f) {
    soc_percent = 100.0f;
  }

  return soc_percent;
}

inline std::string format_minutes_hm(uint32_t total_minutes) {
  uint32_t h = total_minutes / 60;
  uint32_t m = total_minutes % 60;

  char buf[20];
  snprintf(buf, sizeof(buf), "%dh %dm", (int) h, (int) m);
  return std::string(buf);
}

inline std::string format_charging_time(float hours_raw) {
  if (hours_raw <= 0) {
    return "NA";
  }

  return format_minutes_hm((uint32_t)(hours_raw * 60));
}

inline std::string format_duration_hm(uint32_t total_seconds) {
  return format_minutes_hm(total_seconds / 60);
}

inline float get_power_in() {
  if (id(bms_power).state > 0) {
    return id(bms_power).state;
  }

  return 0;
}

inline float get_power_out() {
  if (id(bms_power).state < 0) {
    return id(bms_power).state * -1.0f;
  }

  return 0;
}

// "JBD Charging Active" detection. The BMS's own "charging" flag reflects MOSFET enable
// and can remain true near idle current, so use actual current instead.
// Turns on as soon as current reaches the threshold, but turns off only after it has
// stayed below the threshold for the whole off delay. Short dips must not retrigger
// on_press/on_release, which would reset an active charging session.
inline bool is_charging_active(float threshold_a, uint32_t off_delay_ms) {
  static bool active = false;
  static bool below = false;
  static uint32_t below_since_ms = 0;
  if (!id(bms_a).has_state()) {
    return false;
  }
  const float current_a = id(bms_a).state;
  if (std::isnan(current_a)) {
    return active;
  }
  if (current_a >= threshold_a) {
    active = true;
    below = false;
  } else if (active) {
    const uint32_t now = esphome::millis();
    if (!below) {
      below = true;
      below_since_ms = now;
    } else if (now - below_since_ms >= off_delay_ms) {
      active = false;
      below = false;
    }
  }
  return active;
}

// "JBD Charging Active" turned on.
inline void on_charging_started() {
  // 'first_edge' is a RAM-only static, not an ESPHome global, so it
  // is always true again after a real reboot.
  // First edge after boot:
  // - if charging was already active at boot -> resume (do not reset)
  // - otherwise -> this is a new session (reset to 0)
  // Later edges are always new sessions and must reset.
  static bool first_edge = true;
  const bool was_first_edge = first_edge;
  const bool resuming = was_first_edge && id(bms_charging_was_active_at_boot);
  if (!resuming) {
    id(bms_charging_duration_s) = 0;
  }
  first_edge = false;
  ESP_LOGI("charging_duration", "on_press fired (current=%.2f A), first_edge=%s, active_at_boot=%s, resuming=%s, duration=%u s",
           id(bms_a).state,
           was_first_edge ? "true" : "false",
           id(bms_charging_was_active_at_boot) ? "true" : "false",
           resuming ? "true" : "false",
           (unsigned) id(bms_charging_duration_s));
}

// "JBD Charging Active" turned off: the duration counter stays frozen at its last value.
inline void on_charging_stopped() {
  ESP_LOGI("charging_duration", "on_release fired (current=%.2f A), duration frozen at %u s",
           id(bms_a).state, (unsigned) id(bms_charging_duration_s));
}

inline void set_soc_low_limit(float value) {
  float rounded_value = roundf(value);
  id(soc_low_limit_number).publish_state(rounded_value);
  ESP_LOGI("soc_limits", "SOC Low Limit set to %.0f Ah", rounded_value);
}

inline void set_soc_high_limit(float value) {
  float rounded_value = roundf(value);
  id(soc_high_limit_number).publish_state(rounded_value);
  ESP_LOGI("soc_limits", "SOC High Limit set to %.0f Ah", rounded_value);
}

inline void init_soc_limits(float low_initial, float high_initial, float low_limit_percentage, float high_limit_percentage) {
  if (!id(bms_total_capacity).has_state()) {
    ESP_LOGW("init_limits", "JBD Total Capacity not available");
    return;
  }

  float total_cap = id(bms_total_capacity).state;
  float current_low = id(soc_low_limit_number).state;
  float current_high = id(soc_high_limit_number).state;

  bool low_unchanged = (fabs(current_low - low_initial) < 0.1f);
  bool high_unchanged = (fabs(current_high - high_initial) < 0.1f);

  if (!low_unchanged || !high_unchanged) {
    ESP_LOGI("init_limits", "SOC Limits already configured by user, keeping restored values");
    return;
  }

  float low_limit = roundf(total_cap * low_limit_percentage);
  float high_limit = roundf(total_cap * high_limit_percentage);

  id(soc_low_limit_number).publish_state(low_limit);
  id(soc_high_limit_number).publish_state(high_limit);

  ESP_LOGI("init_limits", "SOC Limits initialized - Total: %.1f, Low: %.0f, High: %.0f", total_cap, low_limit, high_limit);
}

}  // namespace jbdbms_lambdas
