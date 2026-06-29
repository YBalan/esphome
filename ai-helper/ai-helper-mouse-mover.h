#pragma once

#include <cstdint>

#define AI_MOUSE_MOVER_MODE_ENABLED "Enabled"
#define AI_MOUSE_MOVER_MODE_DISABLED "Disabled"

namespace ai_helper {
namespace mouse_mover {

inline const char *mode_text(const bool enabled) {
  return enabled ? AI_MOUSE_MOVER_MODE_ENABLED : AI_MOUSE_MOVER_MODE_DISABLED;
}

inline bool tick_and_should_emit(uint32_t &tick_seconds, const bool enabled, const uint32_t period_seconds) {
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
