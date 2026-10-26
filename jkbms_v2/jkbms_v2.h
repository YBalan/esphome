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

// "BMS Charging Active" detection. The BMS's own "charging" flag reflects the charge
// MOSFET/switch enable bit, which can stay true with near-zero current (charger
// disconnected, battery topped off, CV tail end). Duration tracking needs actual
// current flow, so gate it on current instead.
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

// "BMS Charging Active" turned on.
inline void on_charging_started() {
  // 'first_edge' is a RAM-only static, not an ESPHome global, so it
  // is always true again after a real reboot - unlike a persisted
  // flag it can never get stuck true from an earlier session.
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

// "BMS Charging Active" turned off: the duration counter stays frozen at its last value.
inline void on_charging_stopped() {
  ESP_LOGI("charging_duration", "on_release fired (current=%.2f A), duration frozen at %u s",
           id(bms_a).state, (unsigned) id(bms_charging_duration_s));
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

#ifdef USE_ESP32
#include <esp_attr.h>
#include <esp_system.h>
#include <esp_task_wdt.h>

// Task-watchdog forensics. The production unit has no serial access, so the
// panic text printed when the 30 s task watchdog fires is lost. IDF calls
// esp_task_wdt_isr_user_handler() from the watchdog interrupt just before it
// panics; we save the ESPHome component the main loop was running (the one it
// is stuck in) plus uptime into RTC no-init memory, which survives the reset.
// On the next boot take_last_report() turns it into text for the
// "BMS Last Watchdog" sensor.
//
// Limits: covers the *task* watchdog only (not the interrupt watchdog). ESPHome
// never clears its "current component", so if the loop was starved rather than
// blocked, the component shown is just the last one it started.
namespace jkbms_wdt {

constexpr uint32_t RECORD_MAGIC = 0x57444731;  // "WDG1"

struct Record {
  uint32_t magic;
  uint32_t uptime_s;
  char component[32];
};

// Not initialised at boot: keeps its contents across software/watchdog resets.
static RTC_NOINIT_ATTR Record record;

// Describe the previous reset and consume the record so it is reported once.
inline std::string take_last_report() {
  Record r = record;
  record.magic = 0;
  if (esp_reset_reason() != ESP_RST_TASK_WDT) {
    return "n/a";
  }
  if (r.magic != RECORD_MAGIC) {
    return "unknown (no record)";
  }
  r.component[sizeof(r.component) - 1] = '\0';
  char buf[64];
  snprintf(buf, sizeof(buf), "%s (uptime %us)", r.component, (unsigned) r.uptime_s);
  return std::string(buf);
}

}  // namespace jkbms_wdt

// Runs in interrupt context: no logging, heap allocation or locks here.
extern "C" void esp_task_wdt_isr_user_handler(void) {
  esphome::Component *component = esphome::App.get_current_component();
  const char *name = component != nullptr ? LOG_STR_ARG(component->get_component_log_str()) : "none";
  jkbms_wdt::Record &r = jkbms_wdt::record;
  size_t i = 0;
  for (; i < sizeof(r.component) - 1 && name[i] != '\0'; i++) {
    r.component[i] = name[i];
  }
  r.component[i] = '\0';
  r.uptime_s = esphome::millis() / 1000;
  r.magic = jkbms_wdt::RECORD_MAGIC;
}
#endif  // USE_ESP32
