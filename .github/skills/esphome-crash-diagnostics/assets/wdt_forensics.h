#pragma once

#ifdef USE_ESP32
#include <esp_attr.h>
#include <esp_system.h>
#include <esp_task_wdt.h>

// Task-watchdog forensics for devices with no serial access. IDF calls
// esp_task_wdt_isr_user_handler() from the watchdog interrupt just before it
// panics; we save the ESPHome component the main loop was running (the one it
// is stuck in) plus uptime into RTC no-init memory, which survives the reset.
// On the next boot take_last_report() turns it into text for a text sensor.
//
// Limits: covers the *task* watchdog only (not the interrupt watchdog). ESPHome
// never clears its "current component", so if the loop was starved rather than
// blocked, the component shown is just the last one it started.
//
// Include this file once (esphome: includes:). It defines a global C symbol, so
// including it from a second translation unit would be a duplicate definition.
namespace wdt_forensics {

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

}  // namespace wdt_forensics

// Runs in interrupt context: no logging, heap allocation or locks here.
extern "C" void esp_task_wdt_isr_user_handler(void) {
  esphome::Component *component = esphome::App.get_current_component();
  const char *name = component != nullptr ? LOG_STR_ARG(component->get_component_log_str()) : "none";
  wdt_forensics::Record &r = wdt_forensics::record;
  size_t i = 0;
  for (; i < sizeof(r.component) - 1 && name[i] != '\0'; i++) {
    r.component[i] = name[i];
  }
  r.component[i] = '\0';
  r.uptime_s = esphome::millis() / 1000;
  r.magic = wdt_forensics::RECORD_MAGIC;
}
#endif  // USE_ESP32
