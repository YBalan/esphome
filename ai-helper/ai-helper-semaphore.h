#pragma once

#include <string>
#include <array>

#define AI_SEM_LOG_TAG_AI_STATUS "ai_status"

#define AI_SEM_LOG_STATUS_UNCHANGED "Status unchanged (%d), skipping LED switch"
#define AI_SEM_LOG_STATUS_CHANGED "Status changed: %d -> %d"

#define AI_SEM_STATUS_IDLE "Idle"
#define AI_SEM_STATUS_REASONING "Reasoning"
#define AI_SEM_STATUS_DONE "Done"
#define AI_SEM_STATUS_NEED_USER_ACTION "Need User Action"
#define AI_SEM_STATUS_ALLOW_ACTION "Allow Action"
#define AI_SEM_STATUS_CUSTOM1 "Custom1"
#define AI_SEM_STATUS_CUSTOM2 "Custom2"

#define AI_SEM_COLOR_RED "Red"
#define AI_SEM_COLOR_YELLOW "Yellow"
#define AI_SEM_COLOR_GREEN "Green"
#define AI_SEM_COLOR_BLUE "Blue"
#define AI_SEM_COLOR_WHITE "White"
#define AI_SEM_COLOR_RAINBOW "Rainbow"

#define AI_SEM_LED_MODE_IDLE_OFF "Idle Off"
#define AI_SEM_LED_MODE_WHOLE_STRIP_RAINBOW "Whole Strip Rainbow"
#define AI_SEM_LED_MODE_SINGLE_LED "Single LED"
#define AI_SEM_LED_MODE_OFF "Off"

#define AI_SEM_LED_ASSIGN_RED "Red"
#define AI_SEM_LED_ASSIGN_YELLOW "Yellow"
#define AI_SEM_LED_ASSIGN_GREEN "Green"

#define AI_SEM_DEFAULT_LED_SLOT 0
#define AI_SEM_LED_SLOT_RED 1
#define AI_SEM_LED_SLOT_YELLOW 2
#define AI_SEM_LED_SLOT_GREEN 3

#define AI_SEM_STATUS_CODE_IDLE 0
#define AI_SEM_STATUS_CODE_REASONING 1
#define AI_SEM_STATUS_CODE_DONE 2
#define AI_SEM_STATUS_CODE_NEED_USER_ACTION 3
#define AI_SEM_STATUS_CODE_ALLOW_ACTION 4
#define AI_SEM_STATUS_CODE_CUSTOM1 5
#define AI_SEM_STATUS_CODE_CUSTOM2 6
#define AI_SEM_STATUS_CODE_COUNT 7

#define AI_SEM_DEFAULT_FLOAT_ZERO 0.0f
#define AI_SEM_COLOR_DEFAULT_FULL 1.0f
#define AI_SEM_COLOR_DEFAULT_ZERO 0.0f
#define AI_SEM_COLOR_YELLOW_GREEN 0.78f

#define AI_SEM_PERCENT_DIVISOR 100.0f

#define AI_SEM_DEFAULT_BEEP_ENABLED false
#define AI_SEM_DEFAULT_BEEP_FREQUENCY 1800.0f
#define AI_SEM_DEFAULT_BEEP_REPEAT 1
#define AI_SEM_DEFAULT_BEEP_VOLUME 0.0f
#define AI_SEM_DEFAULT_RAINBOW_EFFECT false

namespace ai_helper {
namespace semaphore {

struct LedConfig {
  std::string assignment;
  std::string color;
  float brightness;
};

struct BuzzerConfig {
  bool enabled;
  float frequency;
  int repeat_count;
  float volume;
};

struct StatusConfig {
  LedConfig led;
  BuzzerConfig buzzer;
};

struct ActiveState {
  int led_slot;
  float led_r;
  float led_g;
  float led_b;
  float led_brightness;
  bool beep_enabled;
  float beep_frequency;
  int beep_repeat;
  float beep_volume;
  bool rainbow_effect;
  bool idle;
};

using StatusConfigArray = std::array<StatusConfig, AI_SEM_STATUS_CODE_COUNT>;

inline int status_code_from_option(const std::string &option) {
  if (option == AI_SEM_STATUS_REASONING) {
    return AI_SEM_STATUS_CODE_REASONING;
  }
  if (option == AI_SEM_STATUS_DONE) {
    return AI_SEM_STATUS_CODE_DONE;
  }
  if (option == AI_SEM_STATUS_NEED_USER_ACTION) {
    return AI_SEM_STATUS_CODE_NEED_USER_ACTION;
  }
  if (option == AI_SEM_STATUS_ALLOW_ACTION) {
    return AI_SEM_STATUS_CODE_ALLOW_ACTION;
  }
  if (option == AI_SEM_STATUS_CUSTOM1) {
    return AI_SEM_STATUS_CODE_CUSTOM1;
  }
  if (option == AI_SEM_STATUS_CUSTOM2) {
    return AI_SEM_STATUS_CODE_CUSTOM2;
  }
  return AI_SEM_STATUS_CODE_IDLE;
}

inline std::string normalized_status(const std::string &raw) {
  if (raw == AI_SEM_STATUS_REASONING) {
    return AI_SEM_STATUS_REASONING;
  }
  if (raw == AI_SEM_STATUS_DONE) {
    return AI_SEM_STATUS_DONE;
  }
  if (raw == AI_SEM_STATUS_NEED_USER_ACTION) {
    return AI_SEM_STATUS_NEED_USER_ACTION;
  }
  if (raw == AI_SEM_STATUS_ALLOW_ACTION) {
    return AI_SEM_STATUS_ALLOW_ACTION;
  }
  if (raw == AI_SEM_STATUS_CUSTOM1) {
    return AI_SEM_STATUS_CUSTOM1;
  }
  if (raw == AI_SEM_STATUS_CUSTOM2) {
    return AI_SEM_STATUS_CUSTOM2;
  }
  return AI_SEM_STATUS_IDLE;
}

inline const char *status_text_from_code(const int code) {
  switch (code) {
    case AI_SEM_STATUS_CODE_REASONING:
      return AI_SEM_STATUS_REASONING;
    case AI_SEM_STATUS_CODE_DONE:
      return AI_SEM_STATUS_DONE;
    case AI_SEM_STATUS_CODE_NEED_USER_ACTION:
      return AI_SEM_STATUS_NEED_USER_ACTION;
    case AI_SEM_STATUS_CODE_ALLOW_ACTION:
      return AI_SEM_STATUS_ALLOW_ACTION;
    case AI_SEM_STATUS_CODE_CUSTOM1:
      return AI_SEM_STATUS_CUSTOM1;
    case AI_SEM_STATUS_CODE_CUSTOM2:
      return AI_SEM_STATUS_CUSTOM2;
    default:
      return AI_SEM_STATUS_IDLE;
  }
}

inline void color_from_name(const std::string &name, float &r, float &g, float &b, bool &rainbow) {
  r = AI_SEM_COLOR_DEFAULT_FULL;
  g = AI_SEM_COLOR_DEFAULT_ZERO;
  b = AI_SEM_COLOR_DEFAULT_ZERO;
  rainbow = false;

  if (name == AI_SEM_COLOR_YELLOW) {
    r = AI_SEM_COLOR_DEFAULT_FULL;
    g = AI_SEM_COLOR_YELLOW_GREEN;
    b = AI_SEM_COLOR_DEFAULT_ZERO;
  } else if (name == AI_SEM_COLOR_GREEN) {
    r = AI_SEM_COLOR_DEFAULT_ZERO;
    g = AI_SEM_COLOR_DEFAULT_FULL;
    b = AI_SEM_COLOR_DEFAULT_ZERO;
  } else if (name == AI_SEM_COLOR_BLUE) {
    r = AI_SEM_COLOR_DEFAULT_ZERO;
    g = AI_SEM_COLOR_DEFAULT_ZERO;
    b = AI_SEM_COLOR_DEFAULT_FULL;
  } else if (name == AI_SEM_COLOR_WHITE) {
    r = AI_SEM_COLOR_DEFAULT_FULL;
    g = AI_SEM_COLOR_DEFAULT_FULL;
    b = AI_SEM_COLOR_DEFAULT_FULL;
  } else if (name == AI_SEM_COLOR_RAINBOW) {
    rainbow = true;
  }
}

inline bool should_apply_status_update(const int current_status_code, const int new_status_code) {
  return new_status_code != current_status_code;
}

inline bool update_status_from_option_and_log(const std::string &option, int &status_code) {
  const int new_status_code = status_code_from_option(option);
  if (!should_apply_status_update(status_code, new_status_code)) {
    ESP_LOGI(AI_SEM_LOG_TAG_AI_STATUS, AI_SEM_LOG_STATUS_UNCHANGED, status_code);
    return false;
  }

  ESP_LOGI(AI_SEM_LOG_TAG_AI_STATUS, AI_SEM_LOG_STATUS_CHANGED, status_code, new_status_code);
  status_code = new_status_code;
  return true;
}

inline const char *led_output_mode_text(const int status_code, const bool active_rainbow_effect, const int active_led_slot) {
  if (status_code == AI_SEM_STATUS_CODE_IDLE) {
    return AI_SEM_LED_MODE_IDLE_OFF;
  }
  if (active_rainbow_effect) {
    return AI_SEM_LED_MODE_WHOLE_STRIP_RAINBOW;
  }
  if (active_led_slot > AI_SEM_DEFAULT_LED_SLOT) {
    return AI_SEM_LED_MODE_SINGLE_LED;
  }
  return AI_SEM_LED_MODE_OFF;
}

inline int led_slot_from_assignment(const std::string &assignment) {
  if (assignment == AI_SEM_LED_ASSIGN_RED) {
    return AI_SEM_LED_SLOT_RED;
  }
  if (assignment == AI_SEM_LED_ASSIGN_YELLOW) {
    return AI_SEM_LED_SLOT_YELLOW;
  }
  if (assignment == AI_SEM_LED_ASSIGN_GREEN) {
    return AI_SEM_LED_SLOT_GREEN;
  }
  return AI_SEM_DEFAULT_LED_SLOT;
}

inline ActiveState compute_active_state(
    const int status_code,
    const StatusConfigArray &configs) {
  ActiveState state{};
  state.led_slot = AI_SEM_DEFAULT_LED_SLOT;
  state.led_r = AI_SEM_DEFAULT_FLOAT_ZERO;
  state.led_g = AI_SEM_DEFAULT_FLOAT_ZERO;
  state.led_b = AI_SEM_DEFAULT_FLOAT_ZERO;
  state.led_brightness = AI_SEM_DEFAULT_FLOAT_ZERO;
  state.beep_enabled = AI_SEM_DEFAULT_BEEP_ENABLED;
  state.beep_frequency = AI_SEM_DEFAULT_BEEP_FREQUENCY;
  state.beep_repeat = AI_SEM_DEFAULT_BEEP_REPEAT;
  state.beep_volume = AI_SEM_DEFAULT_BEEP_VOLUME;
  state.rainbow_effect = AI_SEM_DEFAULT_RAINBOW_EFFECT;
  state.idle = (status_code == AI_SEM_STATUS_CODE_IDLE);

  if (!state.idle && status_code >= 0 && status_code < AI_SEM_STATUS_CODE_COUNT) {
    const auto &config = configs[status_code];
    state.led_slot = led_slot_from_assignment(config.led.assignment);
    state.led_brightness = config.led.brightness / AI_SEM_PERCENT_DIVISOR;
    state.beep_enabled = config.buzzer.enabled;
    state.beep_frequency = config.buzzer.frequency;
    state.beep_repeat = config.buzzer.repeat_count;
    state.beep_volume = config.buzzer.volume;
    color_from_name(config.led.color, state.led_r, state.led_g, state.led_b, state.rainbow_effect);
  }

  return state;
}

inline bool compute_and_assign_active_state(
    int &active_led_slot,
    float &active_led_r,
    float &active_led_g,
    float &active_led_b,
    float &active_led_brightness,
    bool &active_beep_enabled,
    float &active_beep_frequency,
    int &active_beep_repeat,
    float &active_beep_volume,
    bool &active_rainbow_effect,
    const int status_code,
    const StatusConfigArray &configs) {
  const auto state = compute_active_state(status_code, configs);

  active_led_slot = state.led_slot;
  active_led_r = state.led_r;
  active_led_g = state.led_g;
  active_led_b = state.led_b;
  active_led_brightness = state.led_brightness;
  active_beep_enabled = state.beep_enabled;
  active_beep_frequency = state.beep_frequency;
  active_beep_repeat = state.beep_repeat;
  active_beep_volume = state.beep_volume;
  active_rainbow_effect = state.rainbow_effect;

  return state.idle;
}

}  // namespace semaphore
}  // namespace ai_helper
