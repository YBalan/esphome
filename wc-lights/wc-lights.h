#pragma once

#include <cstdint>
#include <string>

#include "esphome/core/time.h"
#include "esphome/core/color.h"
#include "esphome/components/light/addressable_light.h"
#include "esphome/components/light/light_state.h"

namespace wc_lights {

static constexpr uint32_t MS_PER_SECOND = 1000U;
static constexpr uint32_t SECONDS_PER_MINUTE = 60U;
static constexpr uint32_t SECONDS_PER_HOUR = 60U * SECONDS_PER_MINUTE;
static constexpr uint32_t SECONDS_PER_DAY = 24U * SECONDS_PER_HOUR;
static constexpr uint32_t SECONDS_PER_WEEK = 7U * SECONDS_PER_DAY;
static constexpr uint32_t SECONDS_PER_MONTH = 30U * SECONDS_PER_DAY;
static constexpr float PERCENT_TO_UNIT = 0.01f;

inline float pct_to_unit(const int pct) {
  return static_cast<float>(pct) * PERCENT_TO_UNIT;
}

inline bool flash_elapsed(const uint32_t started_ms, const uint32_t now_ms, const int duration_seconds);
inline uint32_t total_time_inside_seconds(
  const uint32_t total_visit_seconds,
  const bool user_inside,
  const uint32_t entry_started_ms,
  const uint32_t now_ms
);
inline std::string format_total_duration(const uint32_t total_seconds);
inline std::string format_visit_duration_hms(const uint32_t total_seconds);
inline bool resolve_endstop_state(const bool raw_state, const std::string &mode);

inline uint32_t seconds_since_ms(const uint32_t start_ms, const uint32_t now_ms) {
  return (now_ms >= start_ms) ? (now_ms - start_ms) : 0U;
}

inline void update_max_seconds(const uint32_t value, uint32_t &current_max) {
  if (value > current_max) {
    current_max = value;
  }
}

inline int minutes_since_midnight(const int hour, const int minute) {
  return hour * 60 + minute;
}

inline bool is_night_time(
  const esphome::ESPTime &now,
  const int start_hour,
  const int start_minute,
  const int end_hour,
  const int end_minute
) {
  if (!now.is_valid()) {
    return false;
  }

  const int current = minutes_since_midnight(now.hour, now.minute);
  const int start = minutes_since_midnight(start_hour, start_minute);
  const int end = minutes_since_midnight(end_hour, end_minute);

  if (start == end) {
    return false;
  }

  if (start < end) {
    return current >= start && current < end;
  }

  return current >= start || current < end;
}

template<typename TLight>
inline void turn_light_on_pct(TLight light, const int pct) {
  auto call = light->turn_on();
  call.set_brightness(pct_to_unit(pct));
  call.perform();
}

template<typename TLight>
inline void handle_room_door_open(
  uint32_t &door_open_started_ms,
  const uint32_t now_ms,
  TLight main_light,
  const esphome::ESPTime &time_now,
  const int day_brightness_pct,
  const int night_brightness_pct,
  const int night_start_hour,
  const int night_start_minute,
  const int night_end_hour,
  const int night_end_minute
) {
  door_open_started_ms = now_ms;

  const bool night_mode = is_night_time(
    time_now,
    night_start_hour,
    night_start_minute,
    night_end_hour,
    night_end_minute
  );

  turn_light_on_pct(main_light, night_mode ? night_brightness_pct : day_brightness_pct);
}

template<typename TLight>
inline void turn_light_off(TLight light) {
  auto call = light->turn_off();
  call.perform();
}

struct RgbPct {
  int red;
  int green;
  int blue;
};

inline RgbPct color_from_preset(const std::string &preset) {
  if (preset == "Red") {
    return {100, 0, 0};
  }
  if (preset == "Green") {
    return {0, 100, 0};
  }
  if (preset == "Blue") {
    return {0, 0, 100};
  }
  if (preset == "Yellow") {
    return {100, 80, 0};
  }
  if (preset == "Orange") {
    return {100, 40, 0};
  }
  if (preset == "Purple") {
    return {60, 0, 100};
  }
  if (preset == "Cyan") {
    return {0, 100, 100};
  }
  if (preset == "White") {
    return {100, 100, 100};
  }
  if (preset == "Warm White") {
    return {100, 70, 30};
  }
  if (preset == "Pink") {
    return {100, 20, 60};
  }

  return {100, 0, 0};
}

inline void update_status_strip(
  esphome::light::AddressableLightState *strip,
  const bool wc_user_inside,
  const bool wc_flash_active,
  const bool bath_flash_active,
  const int wc_inside_brightness_pct,
  const int flash_brightness_pct,
  const int wc_inside_red_pct,
  const int wc_inside_green_pct,
  const int wc_inside_blue_pct,
  const int wc_flash_red_pct,
  const int wc_flash_green_pct,
  const int wc_flash_blue_pct,
  const int bath_flash_red_pct,
  const int bath_flash_green_pct,
  const int bath_flash_blue_pct,
  const int active_led_count
) {
  auto *addr = static_cast<esphome::light::AddressableLight *>(strip->get_output());

  int red_pct = 0;
  int green_pct = 0;
  int blue_pct = 0;
  int brightness_pct = 100;

  if (wc_user_inside) {
    red_pct = wc_inside_red_pct;
    green_pct = wc_inside_green_pct;
    blue_pct = wc_inside_blue_pct;
    brightness_pct = wc_inside_brightness_pct;
  } else if (wc_flash_active) {
    red_pct = wc_flash_red_pct;
    green_pct = wc_flash_green_pct;
    blue_pct = wc_flash_blue_pct;
    brightness_pct = flash_brightness_pct;
  } else if (bath_flash_active) {
    red_pct = bath_flash_red_pct;
    green_pct = bath_flash_green_pct;
    blue_pct = bath_flash_blue_pct;
    brightness_pct = flash_brightness_pct;
  }

  if (brightness_pct < 0) {
    brightness_pct = 0;
  }
  if (brightness_pct > 100) {
    brightness_pct = 100;
  }

  const int total_leds = static_cast<int>(addr->size());
  int active_leds = active_led_count;
  if (active_leds < 0) {
    active_leds = 0;
  }
  if (active_leds > total_leds) {
    active_leds = total_leds;
  }

  const bool strip_should_be_on =
    active_leds > 0 && (red_pct > 0 || green_pct > 0 || blue_pct > 0) && brightness_pct > 0;

  if (!strip_should_be_on) {
    turn_light_off(strip);
    return;
  }

  // Set the light state with the computed color so HA reflects the correct status.
  // ESPHome renders solid color to all LEDs; we then zero out inactive ones.
  auto strip_call = strip->turn_on();
  strip_call.set_effect("None");
  strip_call.set_transition_length(0);
  strip_call.set_red(pct_to_unit(red_pct));
  strip_call.set_green(pct_to_unit(green_pct));
  strip_call.set_blue(pct_to_unit(blue_pct));
  strip_call.set_brightness(pct_to_unit(brightness_pct));
  strip_call.perform();

  // Override pixels beyond active_leds to black (partial-strip effect).
  const esphome::Color off_color(0, 0, 0);
  for (int i = active_leds; i < total_leds; i++) {
    (*addr)[i] = off_color;
  }

  addr->schedule_show();
}

inline void set_flag(bool &flag, const bool value) {
  flag = value;
}

inline void set_test_mode(bool &flag, const bool value) {
  flag = value;
}

inline bool should_skip_status_strip_update(const bool test_mode_active) {
  return test_mode_active;
}

inline int to_int(const float value) {
  return static_cast<int>(value);
}

inline bool to_bool(const bool value) {
  return value;
}

inline float to_brightness(const float value) {
  return value;
}

inline void start_flash(bool &flash_active, uint32_t &flash_started_ms, const uint32_t now_ms) {
  flash_active = true;
  flash_started_ms = now_ms;
}

inline void stop_flash(bool &flash_active) {
  flash_active = false;
}

inline bool should_continue_flash_loop(const uint32_t started_ms, const uint32_t now_ms, const int duration_seconds) {
  return !flash_elapsed(started_ms, now_ms, duration_seconds);
}

template<typename TLight>
inline void start_wc_inside_blink(
  bool &blink_running,
  bool &blink_prev_was_on,
  float &blink_prev_brightness,
  TLight light,
  const int day_brightness_pct
) {
  blink_running = true;
  blink_prev_was_on = light->current_values.is_on();

  float previous_brightness = light->current_values.get_brightness();
  if (previous_brightness <= 0.01f) {
    previous_brightness = pct_to_unit(day_brightness_pct);
  }

  blink_prev_brightness = previous_brightness;
}

inline void finish_wc_inside_blink(bool &blink_running) {
  blink_running = false;
}

template<typename TBlinkScript>
inline void handle_wc_inside_blink_schedule(
  const bool wc_user_inside_flag,
  const bool wc_inside_blink_running,
  uint32_t &wc_inside_blink_next_ms,
  const uint32_t wc_entry_started_ms,
  const int interval_minutes,
  const uint32_t now_ms,
  TBlinkScript wc_inside_blink_script
) {
  if (!wc_user_inside_flag) {
    wc_inside_blink_next_ms = 0;
    return;
  }

  if (wc_inside_blink_running || interval_minutes <= 0) {
    return;
  }

  const uint32_t period_ms = static_cast<uint32_t>(interval_minutes) * SECONDS_PER_MINUTE * MS_PER_SECOND;

  if (wc_inside_blink_next_ms == 0U) {
    wc_inside_blink_next_ms = wc_entry_started_ms + period_ms;
    return;
  }

  if (now_ms >= wc_inside_blink_next_ms) {
    wc_inside_blink_next_ms = now_ms + period_ms;
    wc_inside_blink_script->execute();
  }
}

inline std::string format_total_duration_with_live_room_time(
  const uint32_t total_visit_seconds,
  const bool user_inside,
  const uint32_t entry_started_ms,
  const uint32_t now_ms
) {
  return format_total_duration(total_time_inside_seconds(total_visit_seconds, user_inside, entry_started_ms, now_ms));
}

inline std::string format_visit_duration_from_seconds(const uint32_t seconds) {
  return format_visit_duration_hms(seconds);
}

inline bool resolve_endstop_from_raw(const bool raw_state, const std::string &mode) {
  return resolve_endstop_state(raw_state, mode);
}

inline void update_status_strip_from_presets(
  esphome::light::AddressableLightState *strip,
  const bool test_mode_active,
  const bool wc_user_inside,
  const bool wc_flash_active,
  const bool bath_flash_active,
  const int wc_inside_brightness_pct,
  const int flash_brightness_pct,
  const std::string &wc_inside_color_preset,
  const std::string &wc_flash_color_preset,
  const std::string &bath_flash_color_preset,
  const int active_led_count
) {
  if (should_skip_status_strip_update(test_mode_active)) {
    return;
  }

  const auto wc_inside_color = color_from_preset(wc_inside_color_preset);
  const auto wc_flash_color = color_from_preset(wc_flash_color_preset);
  const auto bath_flash_color = color_from_preset(bath_flash_color_preset);

  update_status_strip(
    strip,
    wc_user_inside,
    wc_flash_active,
    bath_flash_active,
    wc_inside_brightness_pct,
    flash_brightness_pct,
    wc_inside_color.red,
    wc_inside_color.green,
    wc_inside_color.blue,
    wc_flash_color.red,
    wc_flash_color.green,
    wc_flash_color.blue,
    bath_flash_color.red,
    bath_flash_color.green,
    bath_flash_color.blue,
    active_led_count
  );
}

inline bool flash_elapsed(const uint32_t started_ms, const uint32_t now_ms, const int duration_seconds) {
  return (now_ms - started_ms) >= (static_cast<uint32_t>(duration_seconds) * MS_PER_SECOND);
}

inline float room_time_inside_seconds(
  const bool user_inside,
  const uint32_t entry_started_ms,
  const uint32_t now_ms
) {
  if (!user_inside) {
    return 0.0f;
  }
  return static_cast<float>(seconds_since_ms(entry_started_ms, now_ms) / MS_PER_SECOND);
}

inline bool to_binary_state(const bool value) {
  return value;
}

inline float to_float_seconds(const uint32_t value) {
  return static_cast<float>(value);
}

inline uint32_t total_time_inside_seconds(
  const uint32_t total_visit_seconds,
  const bool user_inside,
  const uint32_t entry_started_ms,
  const uint32_t now_ms
) {
  return total_visit_seconds + static_cast<uint32_t>(room_time_inside_seconds(user_inside, entry_started_ms, now_ms));
}

inline std::string pad_two_digits(const uint32_t value) {
  if (value < 10U) {
    return "0" + std::to_string(value);
  }
  return std::to_string(value);
}

inline std::string pluralize_unit(const uint32_t value, const char *singular, const char *plural) {
  return std::to_string(value) + " " + (value == 1U ? singular : plural);
}

inline std::string format_total_duration(const uint32_t total_seconds) {
  uint32_t remaining = total_seconds;

  const uint32_t months = remaining / SECONDS_PER_MONTH;
  remaining %= SECONDS_PER_MONTH;

  const uint32_t weeks = remaining / SECONDS_PER_WEEK;
  remaining %= SECONDS_PER_WEEK;

  const uint32_t days = remaining / SECONDS_PER_DAY;
  remaining %= SECONDS_PER_DAY;

  const uint32_t hours = remaining / SECONDS_PER_HOUR;

  std::string formatted;

  if (months > 0U) {
    formatted += pluralize_unit(months, "month", "months");
  }
  if (weeks > 0U) {
    if (!formatted.empty()) {
      formatted += ", ";
    }
    formatted += pluralize_unit(weeks, "week", "weeks");
  }
  if (days > 0U) {
    if (!formatted.empty()) {
      formatted += ", ";
    }
    formatted += pluralize_unit(days, "day", "days");
  }
  if (hours > 0U || formatted.empty()) {
    if (!formatted.empty()) {
      formatted += ", ";
    }
    formatted += pluralize_unit(hours, "hour", "hours");
  }

  return formatted;
}

inline std::string format_visit_duration_hms(const uint32_t total_seconds) {
  const uint32_t hours = total_seconds / SECONDS_PER_HOUR;
  const uint32_t minutes = (total_seconds % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE;
  const uint32_t seconds = total_seconds % SECONDS_PER_MINUTE;

  return std::to_string(hours) + ":" + pad_two_digits(minutes) + ":" + pad_two_digits(seconds);
}

inline bool resolve_endstop_state(const bool raw_state, const std::string &mode) {
  // With input pull-up wiring, NC and NO contacts map to opposite raw levels.
  return (mode == "NC") ? !raw_state : raw_state;
}

template<typename TLight, typename TFlashScript, typename TStatusScript, typename TBinarySensor, typename TTimeSensor, typename TMaxSensor, typename TLastSensor>
inline void handle_room_door_close(
  const uint32_t now_ms,
  bool &user_inside_flag,
  const uint32_t door_open_started_ms,
  uint32_t &entry_started_ms,
  uint32_t &last_visit_seconds,
  uint32_t &total_visit_seconds,
  uint32_t &max_visit_seconds,
  TLight main_light,
  TFlashScript flash_script,
  TStatusScript status_strip_script,
  TBinarySensor user_inside_sensor,
  TTimeSensor time_inside_sensor,
  TMaxSensor max_time_sensor,
  TLastSensor last_visit_sensor,
  const esphome::ESPTime &time_now,
  const uint32_t open_timeout_seconds,
  const int day_brightness_pct,
  const int night_brightness_pct,
  const int night_start_hour,
  const int night_start_minute,
  const int night_end_hour,
  const int night_end_minute
) {
  const uint32_t open_seconds = seconds_since_ms(door_open_started_ms, now_ms) / MS_PER_SECOND;

  if (!user_inside_flag) {
    if (open_seconds >= open_timeout_seconds) {
      turn_light_off(main_light);
    } else {
      user_inside_flag = true;
      entry_started_ms = now_ms;

      const bool night_mode = is_night_time(
        time_now,
        night_start_hour,
        night_start_minute,
        night_end_hour,
        night_end_minute
      );

      turn_light_on_pct(main_light, night_mode ? night_brightness_pct : day_brightness_pct);
    }
  } else {
    user_inside_flag = false;
    const uint32_t visit_seconds = seconds_since_ms(entry_started_ms, now_ms) / MS_PER_SECOND;
    last_visit_seconds = visit_seconds;
    total_visit_seconds += visit_seconds;
    update_max_seconds(visit_seconds, max_visit_seconds);

    turn_light_off(main_light);
    flash_script->execute();
  }

  user_inside_sensor->publish_state(user_inside_flag);
  time_inside_sensor->publish_state(
    user_inside_flag ? (seconds_since_ms(entry_started_ms, now_ms) / MS_PER_SECOND) : 0
  );
  max_time_sensor->publish_state(max_visit_seconds);
  last_visit_sensor->publish_state(last_visit_seconds);
  status_strip_script->execute();
}

template<typename TSensorA, typename TSensorB, typename TSensorC, typename TSensorD>
inline void reset_all_visit_counters(
  uint32_t &wc_last_visit_seconds,
  uint32_t &bath_last_visit_seconds,
  uint32_t &wc_total_visit_seconds,
  uint32_t &bath_total_visit_seconds,
  uint32_t &wc_max_visit_seconds,
  uint32_t &bath_max_visit_seconds,
  TSensorA wc_last_visit_sensor,
  TSensorB bath_last_visit_sensor,
  TSensorC wc_max_time_inside_sensor,
  TSensorD bath_max_time_inside_sensor
) {
  wc_last_visit_seconds = 0;
  bath_last_visit_seconds = 0;
  wc_total_visit_seconds = 0;
  bath_total_visit_seconds = 0;
  wc_max_visit_seconds = 0;
  bath_max_visit_seconds = 0;
  wc_last_visit_sensor->publish_state(0);
  bath_last_visit_sensor->publish_state(0);
  wc_max_time_inside_sensor->publish_state(0);
  bath_max_time_inside_sensor->publish_state(0);
}

}  // namespace wc_lights
