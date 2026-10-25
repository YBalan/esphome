#pragma once

#include <cmath>
#include <cstdint>
#include <optional>

#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace garage {

// PZEM-004T helpers: reply freshness, stale zeroing and spike rejection.
// Wired from garage-generator-pzem.yaml; the sensor ids (pzem_*) are declared there.

// No PZEM reply for this long -> data is stale (PZEM is powered by the measured line,
// so it goes silent when the generator stops).
inline constexpr uint32_t kPzemMaxAgeMs = 30000U;

// A jump bigger than the guard's max_step is held back until it repeats this many times in a row...
inline constexpr uint8_t kSpikeConfirmSamples = 2U;
// ...or until this many samples in a row were held back (a ramp never repeats, but it is not a spike).
inline constexpr uint8_t kSpikeMaxHeldSamples = 4U;
// Energy only counts up; a drop smaller than this is rounding, not a glitch (resolution is 1 Wh).
inline constexpr float kEnergyDropSlackWh = 0.5f;

inline uint32_t pzem_last_rx_ms = 0U;
inline uint32_t pzem_rejected_count = 0U;
// Set while zero_pzem_sensors_if_stale() publishes, so its zeros skip the spike guards.
inline bool pzem_forcing_zero = false;

// Drops single-sample glitches from one PZEM value.
//  - outside [min, max]         -> garbage, always dropped
//  - within max_step of last    -> accepted
//  - further away               -> held back; accepted once the new level repeats
//                                  (kSpikeConfirmSamples) or persists (kSpikeMaxHeldSamples)
// A real step change is therefore delayed by one poll instead of being lost.
class SpikeGuard {
 public:
  constexpr SpikeGuard(const char *name, const float min, const float max, const float max_step,
                       const bool monotonic = false)
      : name_(name), min_(min), max_(max), max_step_(max_step), monotonic_(monotonic) {}

  // Forget the last good value; the next sample is accepted as the new baseline.
  void reset() {
    have_last_ = false;
    pending_ = 0U;
    held_ = 0U;
  }

  std::optional<float> filter(const float x) {
    if (!std::isfinite(x) || x < min_ || x > max_) {
      reject(x, "out of range");
      return std::nullopt;
    }
    if (!have_last_ || within_step(x)) {
      return accept(x);
    }

    const bool repeats_candidate = pending_ > 0U && std::fabs(x - candidate_) <= max_step_;
    pending_ = repeats_candidate ? static_cast<uint8_t>(pending_ + 1U) : 1U;
    candidate_ = x;
    held_++;
    if (pending_ >= kSpikeConfirmSamples || held_ >= kSpikeMaxHeldSamples) {
      return accept(x);
    }
    reject(x, "spike");
    return std::nullopt;
  }

 private:
  bool within_step(const float x) const {
    const float delta = x - last_;
    if (monotonic_ && delta < -kEnergyDropSlackWh) {
      return false;
    }
    return std::fabs(delta) <= max_step_;
  }

  std::optional<float> accept(const float x) {
    last_ = x;
    have_last_ = true;
    pending_ = 0U;
    held_ = 0U;
    return x;
  }

  void reject(const float x, const char *why) {
    pzem_rejected_count++;
    ESP_LOGW("pzem_guard", "%s: dropped %s sample %.2f (last good %.2f)", name_, why, x, last_);
  }

  const char *name_;
  float min_;
  float max_;
  float max_step_;
  bool monotonic_;
  float last_{0.0f};
  float candidate_{0.0f};
  bool have_last_{false};
  uint8_t pending_{0U};
  uint8_t held_{0U};
};

// Limits are for a PZEM-004T v3 (100 A variant) polled every ~2 s. Raw samples are compared with
// each other, before any throttle, so max_step is "change per poll".
//                                      name         min      max       max_step
inline SpikeGuard pzem_voltage_guard{"voltage", 0.0f, 300.0f, 40.0f};
inline SpikeGuard pzem_current_guard{"current", 0.0f, 100.0f, 20.0f};
inline SpikeGuard pzem_power_guard{"power", 0.0f, 23000.0f, 3000.0f};
inline SpikeGuard pzem_energy_guard{"energy", 0.0f, 9999990.0f, 50.0f, true};
inline SpikeGuard pzem_frequency_guard{"frequency", 40.0f, 70.0f, 5.0f};
inline SpikeGuard pzem_power_factor_guard{"power factor", 0.0f, 1.0f, 0.5f};

inline void reset_pzem_guards() {
  pzem_voltage_guard.reset();
  pzem_current_guard.reset();
  pzem_power_guard.reset();
  pzem_energy_guard.reset();
  pzem_frequency_guard.reset();
  pzem_power_factor_guard.reset();
}

// First filter of every pzemac sensor. Any value that reaches it is a real PZEM reply, so it also
// refreshes the freshness clock (a dropped spike still proves the PZEM is alive).
inline std::optional<float> pzem_spike_filter(SpikeGuard &guard, const float x) {
  if (pzem_forcing_zero) {
    return x;
  }
  pzem_last_rx_ms = millis();
  return guard.filter(x);
}

// pzem_last_rx_ms == 0 means "no reply since boot": not fresh, so the sensors read 0 until the first reply.
inline bool is_pzem_data_fresh(const uint32_t max_age_ms = kPzemMaxAgeMs) {
  if (pzem_last_rx_ms == 0U) {
    return false;
  }
  return static_cast<uint32_t>(millis() - pzem_last_rx_ms) <= max_age_ms;
}

template <typename TSensor>
inline void publish_sensor_zero_if_needed(const float &state, TSensor &sensor) {
  if (!std::isfinite(state) || state != 0.0f) {
    sensor.publish_state(0.0f);
  }
}

// ESPHome keeps the last value when the PZEM stops answering; publish 0 for every value instead.
// Needs the pzem_* sensor ids from garage-generator-pzem.yaml.
inline void zero_pzem_sensors_if_stale(const uint32_t max_age_ms = kPzemMaxAgeMs) {
  if (is_pzem_data_fresh(max_age_ms)) {
    return;
  }

  // The next real reply starts a new baseline instead of being judged against the old value.
  reset_pzem_guards();

  // publish_state() runs the sensor's filter chain, which starts with the spike guard.
  pzem_forcing_zero = true;
  publish_sensor_zero_if_needed(id(pzem_voltage).state, id(pzem_voltage));
  publish_sensor_zero_if_needed(id(pzem_current).state, id(pzem_current));
  publish_sensor_zero_if_needed(id(pzem_power).state, id(pzem_power));
  publish_sensor_zero_if_needed(id(pzem_energy).state, id(pzem_energy));
  publish_sensor_zero_if_needed(id(pzem_frequency).state, id(pzem_frequency));
  publish_sensor_zero_if_needed(id(pzem_power_factor).state, id(pzem_power_factor));
  pzem_forcing_zero = false;
}

}  // namespace garage
