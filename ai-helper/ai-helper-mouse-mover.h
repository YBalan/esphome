#pragma once

#include <cstdio>
#include <cstdint>
#include <cstdlib>

#include <esp_random.h>

#ifndef AI_HELPER_MOUSE_MOVER_BUILD_ENABLED
#define AI_HELPER_MOUSE_MOVER_BUILD_ENABLED 1
#endif

#define AI_MOUSE_MOVER_ANGLE_MIN 0
#define AI_MOUSE_MOVER_ANGLE_90 90
#define AI_MOUSE_MOVER_ANGLE_180 180
#define AI_MOUSE_MOVER_ANGLE_270 270
#define AI_MOUSE_MOVER_LEVEL_RANGE 2.0f
#define AI_MOUSE_MOVER_LEVEL_OFFSET 1.0f
#define AI_MOUSE_MOVER_MS_PER_SECOND 1000U
#define AI_MOUSE_MOVER_DEFAULT_STEP 1
#define AI_MOUSE_MOVER_LCD_LINE_LEN 17
#define AI_MOUSE_MOVER_BOUNDS_TEXT_LEN 32

#define AI_MOUSE_MOVER_MODE_ENABLED "Enabled"
#define AI_MOUSE_MOVER_MODE_DISABLED "Disabled"
#define AI_MOUSE_MOVER_MODE_COMPILE_DISABLED "Compile Disabled"

#define AI_MOUSE_MOVER_TEXT_COMPILE_OFF "MM Compile Off "
#define AI_MOUSE_MOVER_TEXT_ENABLE_IN_BUILD "Enable in build"
#define AI_MOUSE_MOVER_TEXT_BOUND_SAVE_MIN "Hold UP SaveMin "
#define AI_MOUSE_MOVER_TEXT_BOUND_SAVE_MAX "Hold DN SaveMax "
#define AI_MOUSE_MOVER_TEXT_STATUS_ON "ON"
#define AI_MOUSE_MOVER_TEXT_STATUS_OFF "OFF"
#define AI_MOUSE_MOVER_TEXT_CAPTIVE_PORTAL "Captive Portal  "

#define AI_MOUSE_MOVER_FMT_STRING "%s"
#define AI_MOUSE_MOVER_FMT_BOUND_LINE0 "%3d>%3d<%3d"
#define AI_MOUSE_MOVER_FMT_MAIN_LINE0 "%s N:%3lus/%3lu"
#define AI_MOUSE_MOVER_FMT_MAIN_LINE1 "A:%3d S:%2d T:%3d"
#define AI_MOUSE_MOVER_FMT_CAPTIVE_AP "AP:%-13.13s"
#define AI_MOUSE_MOVER_FMT_BOUNDS_TEXT "%d..%d"

namespace ai_helper {
namespace mouse_mover {

inline bool build_enabled() {
  return AI_HELPER_MOUSE_MOVER_BUILD_ENABLED != 0;
}

inline int normalize_max_angle(const int requested_max_angle) {
  if (requested_max_angle <= AI_MOUSE_MOVER_ANGLE_90) {
    return AI_MOUSE_MOVER_ANGLE_90;
  }
  if (requested_max_angle <= AI_MOUSE_MOVER_ANGLE_180) {
    return AI_MOUSE_MOVER_ANGLE_180;
  }
  return AI_MOUSE_MOVER_ANGLE_270;
}

inline int clamp_angle(const int angle, const int max_angle = AI_MOUSE_MOVER_ANGLE_180) {
  const int max_clamped = normalize_max_angle(max_angle);
  if (angle < AI_MOUSE_MOVER_ANGLE_MIN) {
    return AI_MOUSE_MOVER_ANGLE_MIN;
  }
  if (angle > max_clamped) {
    return max_clamped;
  }
  return angle;
}

inline int clamp_between(const int value, const int min_value, const int max_value) {
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return value;
}

inline float angle_to_level(const int angle, const int max_angle = AI_MOUSE_MOVER_ANGLE_180) {
  const int max_clamped = normalize_max_angle(max_angle);
  const int clamped = clamp_angle(angle, max_clamped);
  return (AI_MOUSE_MOVER_LEVEL_RANGE * static_cast<float>(clamped) / static_cast<float>(max_clamped)) - AI_MOUSE_MOVER_LEVEL_OFFSET;
}

inline int normalize_start_angle(const int requested_start, const int end_angle, const int max_angle = AI_MOUSE_MOVER_ANGLE_180) {
  const int max_clamped = normalize_max_angle(max_angle);
  int start = clamp_angle(requested_start, max_clamped);
  const int end = clamp_angle(end_angle, max_clamped);
  if (start >= end) {
    start = end - 1;
  }
  if (start < AI_MOUSE_MOVER_ANGLE_MIN) {
    start = AI_MOUSE_MOVER_ANGLE_MIN;
  }
  return start;
}

inline int normalize_end_angle(const int requested_end, const int start_angle, const int max_angle = AI_MOUSE_MOVER_ANGLE_180) {
  const int max_clamped = normalize_max_angle(max_angle);
  int end = clamp_angle(requested_end, max_clamped);
  const int start = clamp_angle(start_angle, max_clamped);
  if (end <= start) {
    end = start + 1;
  }
  if (end > max_clamped) {
    end = max_clamped;
  }
  return end;
}

inline int normalize_angle_in_range(const int angle, const int start_angle, const int end_angle, const int max_angle = AI_MOUSE_MOVER_ANGLE_180) {
  const int max_clamped = normalize_max_angle(max_angle);
  const int clamped_angle = clamp_angle(angle, max_clamped);
  const int clamped_start = clamp_angle(start_angle, max_clamped);
  const int clamped_end = clamp_angle(end_angle, max_clamped);
  return clamp_between(clamped_angle, clamped_start, clamped_end);
}

inline int normalize_target_angle(const int requested_target, const int start_angle, const int end_angle, const int max_angle = AI_MOUSE_MOVER_ANGLE_180) {
  return normalize_angle_in_range(requested_target, start_angle, end_angle, max_angle);
}

inline int normalize_current_angle(const int current_angle, const int start_angle, const int end_angle, const int max_angle = AI_MOUSE_MOVER_ANGLE_180) {
  return normalize_angle_in_range(current_angle, start_angle, end_angle, max_angle);
}

inline uint32_t elapsed_seconds(const uint32_t now_ms, const uint32_t last_tick_ms) {
  return (now_ms - last_tick_ms) / AI_MOUSE_MOVER_MS_PER_SECOND;
}

inline uint32_t remaining_seconds(const uint32_t now_ms, const uint32_t last_tick_ms, const uint32_t period_seconds) {
  const uint32_t elapsed_s = elapsed_seconds(now_ms, last_tick_ms);
  return elapsed_s >= period_seconds ? 0 : period_seconds - elapsed_s;
}

inline void touch_backlight(uint32_t &backlight_tick_ms, const uint32_t now_ms) {
  backlight_tick_ms = now_ms;
}

inline bool should_backlight_timeout(const uint32_t backlight_tick_ms, const uint32_t now_ms, const uint32_t delay_ms) {
  return backlight_tick_ms > 0 && (now_ms - backlight_tick_ms) >= delay_ms;
}

inline void init_boot_state(uint32_t &last_move_tick_ms, const uint32_t now_ms, bool &runtime_enabled) {
  last_move_tick_ms = now_ms;
  if (!build_enabled()) {
    runtime_enabled = false;
  }
}

inline bool accept_runtime_enable(const bool requested_enable) {
  if (!requested_enable) {
    return false;
  }
  return build_enabled();
}

inline void prepare_cycle(
    const int start_angle,
    const int end_angle,
    const int requested_target,
    const int requested_step,
    const int max_angle,
    int &current_angle,
    int &target_angle,
    int &step) {
  const int max_clamped = normalize_max_angle(max_angle);
  const int start = clamp_angle(start_angle, max_clamped);
  const int end = clamp_angle(end_angle, max_clamped);
  target_angle = normalize_target_angle(requested_target, start, end, max_clamped);
  step = requested_step > 0 ? requested_step : AI_MOUSE_MOVER_DEFAULT_STEP;
  current_angle = start;
}

inline int next_forward_angle(const int current_angle, const int step) {
  return current_angle + (step > 0 ? step : AI_MOUSE_MOVER_DEFAULT_STEP);
}

inline int next_backward_angle(const int current_angle, const int step) {
  return current_angle - (step > 0 ? step : AI_MOUSE_MOVER_DEFAULT_STEP);
}

inline bool should_continue_forward(const int current_angle, const int target_angle) {
  return current_angle <= target_angle;
}

inline bool should_continue_backward(const int current_angle, const int start_angle) {
  return current_angle >= start_angle;
}

inline void reset_after_cycle(int &current_angle, const int start_angle, uint32_t &last_move_tick_ms, const uint32_t now_ms, uint32_t &tick_seconds) {
  current_angle = start_angle;
  last_move_tick_ms = now_ms;
  tick_seconds = 0;
}

inline void enter_bound_menu(bool &bound_menu_active, bool &bound_edit_min, const bool edit_min) {
  if (!bound_menu_active) {
    bound_menu_active = true;
  }
  bound_edit_min = edit_min;
}

inline int next_bound_angle(const int current_angle, const int start_angle, const int end_angle, const int bound_step, const int direction) {
  const int delta = (bound_step > 0 ? bound_step : AI_MOUSE_MOVER_DEFAULT_STEP) * direction;
  return clamp_between(current_angle + delta, start_angle, end_angle);
}

inline bool can_save_min(const int current_angle, const int end_angle) {
  return current_angle < end_angle;
}

inline bool can_save_max(const int current_angle, const int start_angle) {
  return current_angle > start_angle;
}

inline void format_compile_off(char line0[AI_MOUSE_MOVER_LCD_LINE_LEN], char line1[AI_MOUSE_MOVER_LCD_LINE_LEN]) {
  std::snprintf(line0, AI_MOUSE_MOVER_LCD_LINE_LEN, AI_MOUSE_MOVER_FMT_STRING, AI_MOUSE_MOVER_TEXT_COMPILE_OFF);
  std::snprintf(line1, AI_MOUSE_MOVER_LCD_LINE_LEN, AI_MOUSE_MOVER_FMT_STRING, AI_MOUSE_MOVER_TEXT_ENABLE_IN_BUILD);
}

inline void format_bound_screen(
    char line0[AI_MOUSE_MOVER_LCD_LINE_LEN],
    char line1[AI_MOUSE_MOVER_LCD_LINE_LEN],
    const int start_angle,
    const int current_angle,
    const int end_angle,
    const bool edit_min) {
  std::snprintf(line0, AI_MOUSE_MOVER_LCD_LINE_LEN, AI_MOUSE_MOVER_FMT_BOUND_LINE0, start_angle, current_angle, end_angle);
  std::snprintf(
      line1,
      AI_MOUSE_MOVER_LCD_LINE_LEN,
      AI_MOUSE_MOVER_FMT_STRING,
      edit_min ? AI_MOUSE_MOVER_TEXT_BOUND_SAVE_MIN : AI_MOUSE_MOVER_TEXT_BOUND_SAVE_MAX);
}

inline void format_main_screen(
    char line0[AI_MOUSE_MOVER_LCD_LINE_LEN],
    char line1[AI_MOUSE_MOVER_LCD_LINE_LEN],
    const bool enabled,
    const uint32_t remain_seconds,
    const uint32_t period_seconds,
    const int current_angle,
    const int step,
    const int delay_ms) {
  std::snprintf(
      line0,
      AI_MOUSE_MOVER_LCD_LINE_LEN,
      AI_MOUSE_MOVER_FMT_MAIN_LINE0,
      enabled ? AI_MOUSE_MOVER_TEXT_STATUS_ON : AI_MOUSE_MOVER_TEXT_STATUS_OFF,
      (unsigned long) remain_seconds,
      (unsigned long) period_seconds);
  std::snprintf(line1, AI_MOUSE_MOVER_LCD_LINE_LEN, AI_MOUSE_MOVER_FMT_MAIN_LINE1, current_angle, step, delay_ms);
}

inline void format_captive_ap_line(char line1[AI_MOUSE_MOVER_LCD_LINE_LEN], const char *ap_name) {
  if (ap_name == nullptr || ap_name[0] == '\0') {
    std::snprintf(line1, AI_MOUSE_MOVER_LCD_LINE_LEN, AI_MOUSE_MOVER_FMT_STRING, AI_MOUSE_MOVER_TEXT_CAPTIVE_PORTAL);
    return;
  }
  std::snprintf(line1, AI_MOUSE_MOVER_LCD_LINE_LEN, AI_MOUSE_MOVER_FMT_CAPTIVE_AP, ap_name);
}

template<typename DisplayType>
inline void render_display(
    DisplayType &it,
    const bool build_is_enabled,
    const bool bound_menu_active,
    const bool bound_edit_min,
    const int start_angle,
    const int current_angle,
    const int end_angle,
    const bool runtime_enabled,
    const uint32_t now_ms,
    const uint32_t last_move_tick_ms,
    const uint32_t period_seconds,
    const int move_step,
    const int move_delay_ms,
    const bool captive_portal_active,
    const char *captive_ap_name) {
  char line0[AI_MOUSE_MOVER_LCD_LINE_LEN];
  char line1[AI_MOUSE_MOVER_LCD_LINE_LEN];

  if (!build_is_enabled) {
    format_compile_off(line0, line1);
    it.print(0, 0, line0);
    it.print(0, 1, line1);
    return;
  }

  if (bound_menu_active) {
    format_bound_screen(line0, line1, start_angle, current_angle, end_angle, bound_edit_min);
    it.print(0, 0, line0);
    it.print(0, 1, line1);
    return;
  }

  const uint32_t remain_s = remaining_seconds(now_ms, last_move_tick_ms, period_seconds);
  format_main_screen(line0, line1, runtime_enabled, remain_s, period_seconds, current_angle, move_step, move_delay_ms);
  if (captive_portal_active) {
    format_captive_ap_line(line1, captive_ap_name);
  }
  it.print(0, 0, line0);
  it.print(0, 1, line1);
}

template<typename ServoType>
inline void apply_servo_angle(ServoType *servo, const int angle, const int max_angle = AI_MOUSE_MOVER_ANGLE_180) {
  if (servo == nullptr) {
    return;
  }
  servo->write(angle_to_level(angle, max_angle));
}

inline const char *bounds_text(const int start_angle, const int end_angle, char buf[AI_MOUSE_MOVER_BOUNDS_TEXT_LEN]) {
  std::snprintf(buf, AI_MOUSE_MOVER_BOUNDS_TEXT_LEN, AI_MOUSE_MOVER_FMT_BOUNDS_TEXT, start_angle, end_angle);
  return buf;
}

inline int random_local_range(const int min_value, const int max_value) {
  int min_v = min_value;
  int max_v = max_value;
  if (min_v > max_v) {
    const int tmp = min_v;
    min_v = max_v;
    max_v = tmp;
  }
  const uint32_t range = static_cast<uint32_t>(max_v - min_v + 1);
  const uint32_t raw = esp_random();
  return min_v + static_cast<int>(raw % range);
}

inline int randomize_step(const int step_min, const int step_max) {
  return random_local_range(step_min, step_max);
}

inline bool parse_int_response(const char *body, int &value) {
  if (body == nullptr) {
    return false;
  }
  while (*body != '\0' && *body != '-' && (*body < '0' || *body > '9')) {
    ++body;
  }
  if (*body == '\0') {
    return false;
  }
  char *end = nullptr;
  const long parsed = std::strtol(body, &end, 10);
  if (end == body) {
    return false;
  }
  value = static_cast<int>(parsed);
  return true;
}

// Parse up to `count` whitespace/newline-separated integers from `body`.
// Returns how many were successfully parsed (random.org plain col=1 output is
// one integer per line).
inline int parse_int_list(const char *body, int *values, const int count) {
  if (body == nullptr || values == nullptr) {
    return 0;
  }
  int found = 0;
  const char *p = body;
  while (found < count && *p != '\0') {
    while (*p != '\0' && *p != '-' && (*p < '0' || *p > '9')) {
      ++p;
    }
    if (*p == '\0') {
      break;
    }
    char *end = nullptr;
    const long parsed = std::strtol(p, &end, 10);
    if (end == p) {
      break;
    }
    values[found++] = static_cast<int>(parsed);
    p = end;
  }
  return found;
}

// Map a raw uniform draw in [0, raw_max] into [lo, hi] inclusive. Lets us pull
// several values with different target ranges from a single random.org request
// that used one wide range for all of them.
inline int scale_to_range(const long raw, const long raw_max, const int lo, const int hi) {
  int low = lo;
  int high = hi;
  if (low > high) {
    const int tmp = low;
    low = high;
    high = tmp;
  }
  long r = raw;
  if (r < 0) {
    r = 0;
  }
  if (r > raw_max) {
    r = raw_max;
  }
  const long long span = static_cast<long long>(high) - static_cast<long long>(low) + 1;
  const long long scaled = static_cast<long long>(low) + (static_cast<long long>(r) * span) / (raw_max + 1);
  return clamp_between(static_cast<int>(scaled), low, high);
}

inline int start_offset_angle(const int start_angle, const int end_angle) {
  const int diff = end_angle - start_angle;
  return start_angle + (diff / 2);
}

// The four random values produced from a single random.org draw. When ok is
// false the response was unusable and the caller should fall back to local
// randomness.
struct RandomValues {
  bool ok = false;
  int angle = 0;
  int step = 0;
  int speed_ms = 0;
  int period_s = 0;
};

// Parse the 4-integer random.org plain response and scale each raw draw
// (uniform over [0, RAW_MAX]) into its own target range.
inline RandomValues compute_random_values(
    const int status_code,
    const char *body,
    const int start_angle,
    const int end_angle,
    const int step_min,
    const int step_max,
    const int delay_min_ms,
    const int delay_max_ms,
    const int period_min_s,
    const int period_max_s) {
  RandomValues out;
  int raw[4] = {0, 0, 0, 0};
  const long raw_max = 1000000L;
  if (status_code != 200 || parse_int_list(body, raw, 4) != 4) {
    return out;  // ok stays false -> caller uses local values
  }
  const int angle_min = start_offset_angle(start_angle, end_angle);
  out.angle = scale_to_range(raw[0], raw_max, angle_min, end_angle);
  out.step = scale_to_range(raw[1], raw_max, step_min, step_max);
  out.speed_ms = scale_to_range(raw[2], raw_max, delay_min_ms, delay_max_ms);
  out.period_s = scale_to_range(raw[3], raw_max, period_min_s, period_max_s);
  out.ok = true;
  return out;
}

inline const char *mode_text(const bool enabled) {
  if (!build_enabled()) {
    return AI_MOUSE_MOVER_MODE_COMPILE_DISABLED;
  }
  return enabled ? AI_MOUSE_MOVER_MODE_ENABLED : AI_MOUSE_MOVER_MODE_DISABLED;
}

}  // namespace mouse_mover
}  // namespace ai_helper
