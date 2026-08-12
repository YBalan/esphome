#pragma once

// Lambda functions for jkbms_v2 ESPHome configuration
// Note: Display-related includes are provided by ESPHome's lambda context

namespace jkbms_lambdas {

// Calculate charging time remaining (hours)
inline float calculate_charging_time() {
  if (!id(bms_total_capacity).has_state() || !id(bms_a).has_state() || !id(bms_ah).has_state()) {
    return 0;
  }
  
  float current = id(bms_a).state;
  float total_cap = id(bms_total_capacity).state;
  float remaining_cap = id(bms_ah).state;
  
  // If the charge current is greater than 0.1 A (charging is in progress)
  if (current > 0.1) {
    float need_ah = total_cap - remaining_cap;
    if (need_ah <= 0) return 0; // Already charged
    return need_ah / current;   // Hours = capacity / current
  } else {
    return 0; // Not charging
  }
}

// Format a duration given in minutes as human readable (h m)
inline std::string format_minutes_hm(uint32_t total_minutes) {
  uint32_t h = total_minutes / 60;
  uint32_t m = total_minutes % 60;

  char buf[20];
  snprintf(buf, sizeof(buf), "%dh %dm", (int) h, (int) m);
  return std::string(buf);
}

// Format charging time remaining (hours) as human readable (h m)
inline std::string format_charging_time(float hours_raw) {
  if (hours_raw <= 0) {
    return "NA";
  }
  return format_minutes_hm((uint32_t)(hours_raw * 60));
}

// Format an accumulated charging duration (seconds) as human readable (h m)
inline std::string format_duration_hm(uint32_t total_seconds) {
  return format_minutes_hm(total_seconds / 60);
}

// Get power in (charging) in watts
inline float get_power_in() {
  if (id(bms_power).state > 0) {
    return id(bms_power).state;
  } else {
    return 0;
  }
}

// Get power out (discharging) in watts
inline float get_power_out() {
  if (id(bms_power).state < 0) {
    return id(bms_power).state * -1.0;
  } else {
    return 0;
  }
}

// Initialize SOC limits based on actual BMS total capacity
// Only initializes if the current values match the initial_value (user hasn't changed them)
inline void init_soc_limits(float low_initial, float high_initial, 
                           float low_limit_percentage, float high_limit_percentage) {
  if (!id(bms_total_capacity).has_state()) {
    ESP_LOGW("init_limits", "BMS Total Capacity not available");
    return;
  }
  
  float total_cap = id(bms_total_capacity).state;
  float current_low = id(soc_low_limit_number).state;
  float current_high = id(soc_high_limit_number).state;
  
  // Only initialize if values haven't been changed from initial defaults
  // Allow small tolerance for floating point comparison (0.1 Ah)
  bool low_unchanged = (fabs(current_low - low_initial) < 0.1);
  bool high_unchanged = (fabs(current_high - high_initial) < 0.1);
  
  // If user changed at least one value, preserve both restored values.
  if (!low_unchanged || !high_unchanged) {
    ESP_LOGI("init_limits", "SOC Limits already configured by user, keeping restored values");
    return;
  }
  
  // Calculate new limits based on actual capacity
  float low_limit = roundf(total_cap * low_limit_percentage);
  float high_limit = roundf(total_cap * high_limit_percentage);
  
  if (low_unchanged) {
    id(soc_low_limit_number).publish_state(low_limit);
  }
  if (high_unchanged) {
    id(soc_high_limit_number).publish_state(high_limit);
  }
  
  ESP_LOGI("init_limits", "SOC Limits initialized - Total: %.1f, Low: %.1f (%.0f%%), High: %.1f (%.0f%%)", 
           total_cap, low_limit, low_limit_percentage * 100, high_limit, high_limit_percentage * 100);
}

// Log SOC limit changes
inline void log_soc_limit_change(const char* limit_name, float value) {
  ESP_LOGI("soc_limits", "%s set to %.1f Ah", limit_name, value);
}

}  // namespace jkbms_lambdas
