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

inline std::string format_charging_time(float hours_raw) {
  if (hours_raw <= 0) {
    return "NA";
  }

  int total_minutes = (int)(hours_raw * 60);
  int h = total_minutes / 60;
  int m = total_minutes % 60;

  char buf[20];
  snprintf(buf, sizeof(buf), "%dh %dm", h, m);
  return std::string(buf);
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
