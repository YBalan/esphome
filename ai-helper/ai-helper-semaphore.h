#pragma once

#include <string>

#define AI_SEM_LOG_TAG_AI_STATUS "ai_status"

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
#define AI_SEM_DEFAULT_FLOAT_ZERO 0.0f
#define AI_SEM_DEFAULT_BEEP_ENABLED false
#define AI_SEM_DEFAULT_BEEP_FREQUENCY 1800.0f
#define AI_SEM_DEFAULT_BEEP_REPEAT 1
#define AI_SEM_DEFAULT_BEEP_VOLUME 0.0f
#define AI_SEM_DEFAULT_RAINBOW_EFFECT false

namespace ai_helper {
namespace semaphore {

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

inline int status_code_from_option(const std::string &option) {
  if (option == AI_SEM_STATUS_REASONING) {
    return 1;
  }
  if (option == AI_SEM_STATUS_DONE) {
    return 2;
  }
  if (option == AI_SEM_STATUS_NEED_USER_ACTION) {
    return 3;
  }
  if (option == AI_SEM_STATUS_ALLOW_ACTION) {
    return 4;
  }
  if (option == AI_SEM_STATUS_CUSTOM1) {
    return 5;
  }
  if (option == AI_SEM_STATUS_CUSTOM2) {
    return 6;
  }
  return 0;
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
    case 1:
      return AI_SEM_STATUS_REASONING;
    case 2:
      return AI_SEM_STATUS_DONE;
    case 3:
      return AI_SEM_STATUS_NEED_USER_ACTION;
    case 4:
      return AI_SEM_STATUS_ALLOW_ACTION;
    case 5:
      return AI_SEM_STATUS_CUSTOM1;
    case 6:
      return AI_SEM_STATUS_CUSTOM2;
    default:
      return AI_SEM_STATUS_IDLE;
  }
}

inline void color_from_name(const std::string &name, float &r, float &g, float &b, bool &rainbow) {
  r = 1.0f;
  g = 0.0f;
  b = 0.0f;
  rainbow = false;

  if (name == AI_SEM_COLOR_YELLOW) {
    r = 1.0f;
    g = 0.78f;
    b = 0.0f;
  } else if (name == AI_SEM_COLOR_GREEN) {
    r = 0.0f;
    g = 1.0f;
    b = 0.0f;
  } else if (name == AI_SEM_COLOR_BLUE) {
    r = 0.0f;
    g = 0.0f;
    b = 1.0f;
  } else if (name == AI_SEM_COLOR_WHITE) {
    r = 1.0f;
    g = 1.0f;
    b = 1.0f;
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
    ESP_LOGI(AI_SEM_LOG_TAG_AI_STATUS, "Status unchanged (%d), skipping LED switch", status_code);
    return false;
  }

  ESP_LOGI(AI_SEM_LOG_TAG_AI_STATUS, "Status changed: %d -> %d", status_code, new_status_code);
  status_code = new_status_code;
  return true;
}

inline const char *led_output_mode_text(const int status_code, const bool active_rainbow_effect, const int active_led_slot) {
  if (status_code == 0) {
    return AI_SEM_LED_MODE_IDLE_OFF;
  }
  if (active_rainbow_effect) {
    return AI_SEM_LED_MODE_WHOLE_STRIP_RAINBOW;
  }
  if (active_led_slot > 0) {
    return AI_SEM_LED_MODE_SINGLE_LED;
  }
  return AI_SEM_LED_MODE_OFF;
}

inline int led_slot_from_assignment(const std::string &assignment) {
  if (assignment == AI_SEM_LED_ASSIGN_RED) {
    return 1;
  }
  if (assignment == AI_SEM_LED_ASSIGN_YELLOW) {
    return 2;
  }
  if (assignment == AI_SEM_LED_ASSIGN_GREEN) {
    return 3;
  }
  return AI_SEM_DEFAULT_LED_SLOT;
}

inline ActiveState compute_active_state(
    const int status_code,
    const std::string &reasoning_led_assignment,
    const std::string &reasoning_color,
    const float reasoning_brightness,
    const bool reasoning_buzzer_enabled,
    const float reasoning_buzzer_frequency,
    const int reasoning_buzzer_repeat,
    const float reasoning_buzzer_volume,
    const std::string &done_led_assignment,
    const std::string &done_color,
    const float done_brightness,
    const bool done_buzzer_enabled,
    const float done_buzzer_frequency,
    const int done_buzzer_repeat,
    const float done_buzzer_volume,
    const std::string &need_action_led_assignment,
    const std::string &need_action_color,
    const float need_action_brightness,
    const bool need_action_buzzer_enabled,
    const float need_action_buzzer_frequency,
    const int need_action_buzzer_repeat,
    const float need_action_buzzer_volume,
    const std::string &allow_action_led_assignment,
    const std::string &allow_action_color,
    const float allow_action_brightness,
    const bool allow_action_buzzer_enabled,
    const float allow_action_buzzer_frequency,
    const int allow_action_buzzer_repeat,
    const float allow_action_buzzer_volume,
    const std::string &custom1_led_assignment,
    const std::string &custom1_color,
    const float custom1_brightness,
    const bool custom1_buzzer_enabled,
    const float custom1_buzzer_frequency,
    const int custom1_buzzer_repeat,
    const float custom1_buzzer_volume,
    const std::string &custom2_led_assignment,
    const std::string &custom2_color,
    const float custom2_brightness,
    const bool custom2_buzzer_enabled,
    const float custom2_buzzer_frequency,
    const int custom2_buzzer_repeat,
    const float custom2_buzzer_volume) {
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
  state.idle = (status_code == 0);

  std::string color_name = AI_SEM_COLOR_RED;
  if (status_code == 1) {
    state.led_slot = led_slot_from_assignment(reasoning_led_assignment);
    color_name = reasoning_color;
    state.led_brightness = reasoning_brightness / 100.0f;
    state.beep_enabled = reasoning_buzzer_enabled;
    state.beep_frequency = reasoning_buzzer_frequency;
    state.beep_repeat = reasoning_buzzer_repeat;
    state.beep_volume = reasoning_buzzer_volume;
  } else if (status_code == 2) {
    state.led_slot = led_slot_from_assignment(done_led_assignment);
    color_name = done_color;
    state.led_brightness = done_brightness / 100.0f;
    state.beep_enabled = done_buzzer_enabled;
    state.beep_frequency = done_buzzer_frequency;
    state.beep_repeat = done_buzzer_repeat;
    state.beep_volume = done_buzzer_volume;
  } else if (status_code == 3) {
    state.led_slot = led_slot_from_assignment(need_action_led_assignment);
    color_name = need_action_color;
    state.led_brightness = need_action_brightness / 100.0f;
    state.beep_enabled = need_action_buzzer_enabled;
    state.beep_frequency = need_action_buzzer_frequency;
    state.beep_repeat = need_action_buzzer_repeat;
    state.beep_volume = need_action_buzzer_volume;
  } else if (status_code == 4) {
    state.led_slot = led_slot_from_assignment(allow_action_led_assignment);
    color_name = allow_action_color;
    state.led_brightness = allow_action_brightness / 100.0f;
    state.beep_enabled = allow_action_buzzer_enabled;
    state.beep_frequency = allow_action_buzzer_frequency;
    state.beep_repeat = allow_action_buzzer_repeat;
    state.beep_volume = allow_action_buzzer_volume;
  } else if (status_code == 5) {
    state.led_slot = led_slot_from_assignment(custom1_led_assignment);
    color_name = custom1_color;
    state.led_brightness = custom1_brightness / 100.0f;
    state.beep_enabled = custom1_buzzer_enabled;
    state.beep_frequency = custom1_buzzer_frequency;
    state.beep_repeat = custom1_buzzer_repeat;
    state.beep_volume = custom1_buzzer_volume;
  } else if (status_code == 6) {
    state.led_slot = led_slot_from_assignment(custom2_led_assignment);
    color_name = custom2_color;
    state.led_brightness = custom2_brightness / 100.0f;
    state.beep_enabled = custom2_buzzer_enabled;
    state.beep_frequency = custom2_buzzer_frequency;
    state.beep_repeat = custom2_buzzer_repeat;
    state.beep_volume = custom2_buzzer_volume;
  }

  if (!state.idle) {
    color_from_name(color_name, state.led_r, state.led_g, state.led_b, state.rainbow_effect);
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
  const std::string &reasoning_led_assignment,
    const std::string &reasoning_color,
    const float reasoning_brightness,
    const bool reasoning_buzzer_enabled,
    const float reasoning_buzzer_frequency,
    const int reasoning_buzzer_repeat,
    const float reasoning_buzzer_volume,
  const std::string &done_led_assignment,
    const std::string &done_color,
    const float done_brightness,
    const bool done_buzzer_enabled,
    const float done_buzzer_frequency,
    const int done_buzzer_repeat,
    const float done_buzzer_volume,
  const std::string &need_action_led_assignment,
    const std::string &need_action_color,
    const float need_action_brightness,
    const bool need_action_buzzer_enabled,
    const float need_action_buzzer_frequency,
    const int need_action_buzzer_repeat,
    const float need_action_buzzer_volume,
  const std::string &allow_action_led_assignment,
    const std::string &allow_action_color,
    const float allow_action_brightness,
    const bool allow_action_buzzer_enabled,
    const float allow_action_buzzer_frequency,
    const int allow_action_buzzer_repeat,
    const float allow_action_buzzer_volume,
  const std::string &custom1_led_assignment,
    const std::string &custom1_color,
    const float custom1_brightness,
    const bool custom1_buzzer_enabled,
    const float custom1_buzzer_frequency,
    const int custom1_buzzer_repeat,
    const float custom1_buzzer_volume,
  const std::string &custom2_led_assignment,
    const std::string &custom2_color,
    const float custom2_brightness,
    const bool custom2_buzzer_enabled,
    const float custom2_buzzer_frequency,
    const int custom2_buzzer_repeat,
    const float custom2_buzzer_volume) {
  const auto state = compute_active_state(
      status_code,
      reasoning_led_assignment,
      reasoning_color,
      reasoning_brightness,
      reasoning_buzzer_enabled,
      reasoning_buzzer_frequency,
      reasoning_buzzer_repeat,
      reasoning_buzzer_volume,
      done_led_assignment,
      done_color,
      done_brightness,
      done_buzzer_enabled,
      done_buzzer_frequency,
      done_buzzer_repeat,
      done_buzzer_volume,
      need_action_led_assignment,
      need_action_color,
      need_action_brightness,
      need_action_buzzer_enabled,
      need_action_buzzer_frequency,
      need_action_buzzer_repeat,
      need_action_buzzer_volume,
      allow_action_led_assignment,
      allow_action_color,
      allow_action_brightness,
      allow_action_buzzer_enabled,
      allow_action_buzzer_frequency,
      allow_action_buzzer_repeat,
      allow_action_buzzer_volume,
      custom1_led_assignment,
      custom1_color,
      custom1_brightness,
      custom1_buzzer_enabled,
      custom1_buzzer_frequency,
      custom1_buzzer_repeat,
      custom1_buzzer_volume,
      custom2_led_assignment,
      custom2_color,
      custom2_brightness,
      custom2_buzzer_enabled,
      custom2_buzzer_frequency,
      custom2_buzzer_repeat,
      custom2_buzzer_volume);

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
