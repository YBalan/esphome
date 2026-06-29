#pragma once

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#include <esp_random.h>
#include <esp_tls.h>
#include <esp_crt_bundle.h>

#ifndef AI_HELPER_MOUSE_MOVER_BUILD_ENABLED
#define AI_HELPER_MOUSE_MOVER_BUILD_ENABLED 1
#endif

#define AI_MOUSE_MOVER_MODE_ENABLED "Enabled"
#define AI_MOUSE_MOVER_MODE_DISABLED "Disabled"
#define AI_MOUSE_MOVER_MODE_COMPILE_DISABLED "Compile Disabled"

namespace ai_helper {
namespace mouse_mover {

inline bool build_enabled() {
  return AI_HELPER_MOUSE_MOVER_BUILD_ENABLED != 0;
}

inline int clamp_angle(const int angle) {
  if (angle < 0) {
    return 0;
  }
  if (angle > 180) {
    return 180;
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

inline float angle_to_level(const int angle) {
  const int clamped = clamp_angle(angle);
  return (static_cast<float>(clamped) / 90.0f) - 1.0f;
}

inline int normalize_start_angle(const int requested_start, const int end_angle) {
  int start = clamp_angle(requested_start);
  const int end = clamp_angle(end_angle);
  if (start >= end) {
    start = end - 1;
  }
  if (start < 0) {
    start = 0;
  }
  return start;
}

inline int normalize_end_angle(const int requested_end, const int start_angle) {
  int end = clamp_angle(requested_end);
  const int start = clamp_angle(start_angle);
  if (end <= start) {
    end = start + 1;
  }
  if (end > 180) {
    end = 180;
  }
  return end;
}

inline int normalize_target_angle(const int requested_target, const int start_angle, const int end_angle) {
  return clamp_between(clamp_angle(requested_target), clamp_angle(start_angle), clamp_angle(end_angle));
}

inline int normalize_current_angle(const int current_angle, const int start_angle, const int end_angle) {
  return clamp_between(clamp_angle(current_angle), clamp_angle(start_angle), clamp_angle(end_angle));
}

inline uint32_t elapsed_seconds(const uint32_t now_ms, const uint32_t last_tick_ms) {
  return (now_ms - last_tick_ms) / 1000;
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

inline bool tick_and_should_emit(uint32_t &tick_seconds, const bool enabled, const uint32_t period_seconds);

inline bool should_emit_now(uint32_t &tick_seconds, const bool enabled, const uint32_t period_seconds, const bool bound_menu_active) {
  if (!tick_and_should_emit(tick_seconds, enabled, period_seconds)) {
    return false;
  }
  return !bound_menu_active;
}

inline void prepare_cycle(
    const int start_angle,
    const int end_angle,
    const int requested_target,
    const int requested_step,
    int &current_angle,
    int &target_angle,
    int &step) {
  const int start = clamp_angle(start_angle);
  const int end = clamp_angle(end_angle);
  target_angle = normalize_target_angle(requested_target, start, end);
  step = requested_step > 0 ? requested_step : 1;
  current_angle = start;
}

inline int next_forward_angle(const int current_angle, const int step) {
  return current_angle + (step > 0 ? step : 1);
}

inline int next_backward_angle(const int current_angle, const int step) {
  return current_angle - (step > 0 ? step : 1);
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
  const int delta = (bound_step > 0 ? bound_step : 1) * direction;
  return clamp_between(current_angle + delta, start_angle, end_angle);
}

inline bool can_save_min(const int current_angle, const int end_angle) {
  return current_angle < end_angle;
}

inline bool can_save_max(const int current_angle, const int start_angle) {
  return current_angle > start_angle;
}

inline void format_compile_off(char line0[17], char line1[17]) {
  std::snprintf(line0, 17, "%s", "MM Compile Off ");
  std::snprintf(line1, 17, "%s", "Enable in build");
}

inline void format_bound_screen(char line0[17], char line1[17], const int start_angle, const int current_angle, const int end_angle, const bool edit_min) {
  std::snprintf(line0, 17, "%3d>%3d<%3d", start_angle, current_angle, end_angle);
  std::snprintf(line1, 17, "%s", edit_min ? "Hold UP SaveMin " : "Hold DN SaveMax ");
}

inline void format_main_screen(
    char line0[17],
    char line1[17],
    const bool enabled,
    const uint32_t remain_seconds,
    const uint32_t period_seconds,
    const int current_angle,
    const int step,
    const int delay_ms) {
  std::snprintf(line0, 17, "%s N:%3lus/%3lu", enabled ? "MM ON " : "MM OFF", (unsigned long) remain_seconds, (unsigned long) period_seconds);
  std::snprintf(line1, 17, "A:%3d S:%2d T:%3d", current_angle, step, delay_ms);
}

inline void format_captive_ap_line(char line1[17], const char *ap_name) {
  if (ap_name == nullptr || ap_name[0] == '\0') {
    std::snprintf(line1, 17, "%s", "Captive Portal  ");
    return;
  }
  std::snprintf(line1, 17, "AP:%-13.13s", ap_name);
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
  char line0[17];
  char line1[17];

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
inline void apply_servo_angle(ServoType *servo, const int angle) {
  if (servo == nullptr) {
    return;
  }
  servo->write(angle_to_level(angle));
}

inline const char *bounds_text(const int start_angle, const int end_angle, char buf[32]) {
  std::snprintf(buf, 32, "%d..%d", start_angle, end_angle);
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

inline uint32_t mix_seed(uint32_t seed) {
  seed ^= (seed << 13);
  seed ^= (seed >> 17);
  seed ^= (seed << 5);
  return seed;
}

inline int map_seed_to_range(const uint32_t seed, const int min_value, const int max_value) {
  int min_v = min_value;
  int max_v = max_value;
  if (min_v > max_v) {
    const int tmp = min_v;
    min_v = max_v;
    max_v = tmp;
  }
  const uint32_t range = static_cast<uint32_t>(max_v - min_v + 1);
  return min_v + static_cast<int>(seed % range);
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

inline int parse_http_status_code(const char *response) {
  if (response == nullptr) {
    return -1;
  }
  int status = -1;
  std::sscanf(response, "HTTP/%*d.%*d %d", &status);
  return status;
}

inline const char *find_http_body(const char *response) {
  if (response == nullptr) {
    return nullptr;
  }
  const char *split = std::strstr(response, "\r\n\r\n");
  if (split != nullptr) {
    return split + 4;
  }
  split = std::strstr(response, "\n\n");
  if (split != nullptr) {
    return split + 2;
  }
  return nullptr;
}

inline bool fetch_random_org_int(const int min_value, const int max_value, int &value, int &http_status, std::string &status_text) {
  constexpr const char *url_prefix = "https://www.random.org/integers/?num=1&col=1&base=10&format=plain&rnd=new";

  char url[256];
  std::snprintf(url, sizeof(url), "%s&min=%d&max=%d", url_prefix, min_value, max_value);

  esp_tls_cfg_t cfg = {};
  cfg.crt_bundle_attach = esp_crt_bundle_attach;
  cfg.timeout_ms = 1200;

  esp_tls_t *tls = esp_tls_init();
  if (tls == nullptr) {
    http_status = -1;
    status_text = "tls init failed";
    return false;
  }

  if (esp_tls_conn_http_new_sync(url, &cfg, tls) != 1) {
    esp_tls_conn_destroy(tls);
    http_status = -1;
    status_text = "tls connect failed";
    return false;
  }

  char request[320];
  std::snprintf(
      request,
      sizeof(request),
      "GET /integers/?num=1&min=%d&max=%d&col=1&base=10&format=plain&rnd=new HTTP/1.0\r\n"
      "Host: www.random.org\r\n"
      "User-Agent: esphome-ai-helper\r\n"
      "Accept: text/plain\r\n"
      "Accept-Encoding: identity\r\n"
      "Connection: close\r\n\r\n",
      min_value,
      max_value);

  const size_t req_len = std::strlen(request);
  const int written = static_cast<int>(esp_tls_conn_write(tls, request, req_len));
  if (written <= 0) {
    esp_tls_conn_destroy(tls);
    http_status = -1;
    status_text = "tls write failed";
    return false;
  }
  yield();

  char response[768];
  std::memset(response, 0, sizeof(response));
  int total = 0;
  while (total < static_cast<int>(sizeof(response) - 1)) {
    const int n = static_cast<int>(esp_tls_conn_read(tls, response + total, sizeof(response) - 1 - total));
    if (n <= 0) {
      break;
    }
    total += n;
    yield();
  }

  response[total] = '\0';
  http_status = parse_http_status_code(response);
  if (http_status != 200) {
    status_text = "http " + std::to_string(http_status);
    esp_tls_conn_destroy(tls);
    return false;
  }

  const char *body = find_http_body(response);
  if (body == nullptr) {
    status_text = "body missing";
    esp_tls_conn_destroy(tls);
    return false;
  }

  if (!parse_int_response(body, value)) {
    status_text = "parse error";
    esp_tls_conn_destroy(tls);
    return false;
  }

  status_text = "random.org";
  esp_tls_conn_destroy(tls);
  return true;
}

inline bool pick_random_seed(const bool prefer_random_org, uint32_t &seed, int &http_status, std::string &status_text) {
  if (prefer_random_org) {
    int remote_value = 0;
    if (fetch_random_org_int(0, 1000000000, remote_value, http_status, status_text)) {
      seed = static_cast<uint32_t>(remote_value);
      return true;
    }
    seed = esp_random();
    status_text = "fallback local";
    return false;
  }

  seed = esp_random();
  http_status = 0;
  status_text = "local";
  return true;
}

inline bool pick_random_value(const bool prefer_random_org, const int min_value, const int max_value, int &value, int &http_status, std::string &status_text) {
  if (prefer_random_org) {
    if (fetch_random_org_int(min_value, max_value, value, http_status, status_text)) {
      return true;
    }
    value = random_local_range(min_value, max_value);
    status_text = "fallback local";
    return false;
  }

  value = random_local_range(min_value, max_value);
  http_status = 0;
  status_text = "local";
  return true;
}

inline int start_offset_angle(const int start_angle, const int end_angle) {
  const int diff = end_angle - start_angle;
  return start_angle + (diff / 2);
}

inline const char *mode_text(const bool enabled) {
  if (!build_enabled()) {
    return AI_MOUSE_MOVER_MODE_COMPILE_DISABLED;
  }
  return enabled ? AI_MOUSE_MOVER_MODE_ENABLED : AI_MOUSE_MOVER_MODE_DISABLED;
}

inline bool tick_and_should_emit(uint32_t &tick_seconds, const bool enabled, const uint32_t period_seconds) {
  if (!build_enabled()) {
    tick_seconds = 0;
    return false;
  }

  if (!enabled) {
    tick_seconds = 0;
    return false;
  }

  tick_seconds += 1;
  if (tick_seconds >= period_seconds) {
    tick_seconds = 0;
    return true;
  }

  return false;
}

}  // namespace mouse_mover
}  // namespace ai_helper
