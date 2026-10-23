#pragma once

#include <cmath>
#include <cstdint>

#include "esphome/core/helpers.h"

namespace grid_meter {

inline uint32_t now_ms() {
  return millis();
}

// Called from the voltage sensor's on_value (i.e. a real PZEM reply).
inline void mark_pzem_rx_if_finite(const float &value) {
  if (std::isfinite(value)) {
    id(pzem_last_rx_ms) = now_ms();
  }
}

// pzem_last_rx_ms == 0 means "nothing received since boot", so now_ms() - 0
// gives the PZEM the same grace window after boot to answer before it is
// treated as stale (otherwise a healthy meter would publish zeros on every boot).
inline bool is_pzem_data_fresh(const uint32_t max_age_ms) {
  return static_cast<uint32_t>(now_ms() - id(pzem_last_rx_ms)) <= max_age_ms;
}

template <typename TSensor>
inline void publish_sensor_zero_if_needed(const float &state, TSensor &sensor) {
  if (!std::isfinite(state) || state != 0.0f) {
    sensor.publish_state(0.0f);
  }
}

// The PZEM-004T is powered from the measured mains line, so when the grid drops
// it stops answering and ESPHome keeps the last values. Publish 0 for every
// value once no reply has arrived for max_age_ms.
inline void zero_pzem_sensors_if_stale(const uint32_t max_age_ms) {
  if (is_pzem_data_fresh(max_age_ms)) {
    return;
  }

  // Publishing the 0 V voltage fires its on_value -> mark_pzem_rx_if_finite and
  // would make the data look fresh again. Restore the timestamp afterwards so a
  // throttled sensor that dropped its zero is retried on the next tick.
  const uint32_t last_rx_ms = id(pzem_last_rx_ms);
  publish_sensor_zero_if_needed(id(pzem_voltage).state, id(pzem_voltage));
  publish_sensor_zero_if_needed(id(pzem_current).state, id(pzem_current));
  publish_sensor_zero_if_needed(id(pzem_power).state, id(pzem_power));
  publish_sensor_zero_if_needed(id(pzem_energy).state, id(pzem_energy));
  publish_sensor_zero_if_needed(id(pzem_frequency).state, id(pzem_frequency));
  publish_sensor_zero_if_needed(id(pzem_power_factor).state, id(pzem_power_factor));
  id(pzem_last_rx_ms) = last_rx_ms;
}

}  // namespace grid_meter
