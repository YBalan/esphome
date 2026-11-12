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
#include <esp_app_desc.h>
#include <esp_attr.h>
#include <esp_cpu_utils.h>
#include <esp_debug_helpers.h>
#include <esp_memory_utils.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <esp_private/freertos_debug.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <xtensa_context.h>

// Task-watchdog forensics. The production unit has no serial access, so the
// panic text printed when the 30 s task watchdog fires is lost. IDF calls
// esp_task_wdt_isr_user_handler() from the watchdog interrupt just before it
// panics. There we save into RTC no-init memory, which survives the reset:
//  - the ESPHome component the main loop was running (the one it is stuck in)
//    plus uptime, and
//  - the saved call stack of loopTask and of the Bluetooth tasks. The panic
//    backtrace ESPHome records belongs to whatever was running on the crashed
//    core (usually the idle task), not to the task that stopped feeding the
//    watchdog; this is the stack of the task that is actually stuck, e.g. a
//    Bluetooth API call waiting for room in the BTC task's queue.
// On the next boot load_previous() reads the record and last_report(),
// loop_trace() and task_traces() turn it into text for the "BMS Last Watchdog*"
// sensors. Decode the addresses with tools/decode_panic.py (loop_trace() starts
// with the elf= id so the right ELF is picked from elf-archive/).
//
// Limits: covers the *task* watchdog only (not the interrupt watchdog). ESPHome
// never clears its "current component", so if the loop was starved rather than
// blocked, the component shown is just the last one it started. The saved stack is
// where a task last gave up the CPU: exact for a task that is waiting (the top
// frames are then a kernel wait such as vTaskDelay / xQueueReceive / osi_sem_take),
// but only as old as the last context switch for a task that was running on the
// other core when the interrupt fired. (Checked on the device: a task blocked in
// vTaskDelay is saved as an interrupt-style frame, so the frame type says nothing
// about whether it was waiting; read the top frames.)
namespace jkbms_wdt {

constexpr uint32_t RECORD_MAGIC = 0x57444733;  // "WDG3"
constexpr size_t N_TASKS = 5;                  // loopTask + the Bluetooth host/controller tasks
constexpr size_t LOOP_FRAMES = 16;             // loopTask: the stack that shows where it waits
constexpr size_t BT_FRAMES = 6;                // Bluetooth tasks: wait primitive and who called it
constexpr size_t MAX_STATE_LEN = 250;          // Home Assistant drops states longer than 255
constexpr const char *TASK_NAMES[N_TASKS] = {"loopTask", "BTC_TASK", "BTU_TASK", "hciT", "btController"};

// Outcome of capturing one task, kept so an empty trace explains itself.
enum : uint8_t {
  TRACE_NO_HANDLE = 0,    // the task was not found (does not exist yet, or Bluetooth is off)
  TRACE_SOLICITED = 1,    // read from a solicited frame
  TRACE_INTERRUPT = 2,    // read from an interrupt frame (what a waiting task shows on this IDF)
  TRACE_NO_SNAPSHOT = 3,  // the task control block could not be read
  TRACE_BAD_STACK = 4,    // the saved stack pointer was not a sane stack address
};

struct Trace {
  uint8_t depth;  // addresses in pc[]
  uint8_t kind;   // TRACE_*
  uint32_t pc[LOOP_FRAMES];
};

struct Record {
  uint32_t magic;
  uint32_t uptime_s;
  char component[32];
  Trace task[N_TASKS];
};

// Not initialised at boot: keeps its contents across software/watchdog resets.
static RTC_NOINIT_ATTR Record record;

// Task handles, looked up from task context: the watchdog interrupt must not search the
// task lists. nullptr while a task does not exist (Bluetooth off).
static TaskHandle_t handles[N_TASKS];

inline void refresh_tasks() {
  for (size_t i = 0; i < N_TASKS; i++) {
    handles[i] = xTaskGetHandle(TASK_NAMES[i]);
  }
}

// Interrupt context: no logging, heap allocation or locks. vTaskGetSnapshot() only reads
// the TCB, and the frame is walked the same way IDF's core dump and gdbstub do it.
inline void capture_task(Trace &t, TaskHandle_t handle, size_t max_frames) {
  t.depth = 0;
  t.kind = TRACE_NO_HANDLE;
  if (handle == nullptr) {
    return;
  }
  TaskSnapshot_t snap = {};
  if (vTaskGetSnapshot(handle, &snap) != pdTRUE || !esp_ptr_in_dram(snap.pxTopOfStack)) {
    t.kind = TRACE_NO_SNAPSHOT;
    return;
  }
  esp_backtrace_frame_t f = {};
  const XtSolFrame *sol = reinterpret_cast<const XtSolFrame *>(snap.pxTopOfStack);
  if (sol->exit == 0) {  // solicited frame: the task called into the kernel and is waiting
    t.kind = TRACE_SOLICITED;
    f.pc = sol->pc;
    f.sp = sol->a1;
    f.next_pc = sol->a0;
  } else {  // interrupt frame: the task was preempted
    const XtExcFrame *exc = reinterpret_cast<const XtExcFrame *>(snap.pxTopOfStack);
    t.kind = TRACE_INTERRUPT;
    f.pc = exc->pc;
    f.sp = exc->a1;
    f.next_pc = exc->a0;
    f.exc_frame = exc;
  }
  if (!esp_stack_ptr_is_sane(f.sp)) {
    t.kind = TRACE_BAD_STACK;
    return;
  }
  // A task that is waiting is saved inside the context-switch code: its first frame is a
  // ROM address, which says nothing. Drop leading ROM frames. The bound keeps a damaged
  // stack from looping.
  uint32_t pc = esp_cpu_process_stack_pc(f.pc);
  for (size_t step = 0; step < max_frames + 4; step++) {
    if (t.depth > 0 || !esp_ptr_in_rom(reinterpret_cast<const void *>(pc))) {
      t.pc[t.depth++] = pc;
    }
    if (t.depth >= max_frames || f.next_pc == 0 || !esp_backtrace_get_next_frame(&f)) {
      break;
    }
    pc = esp_cpu_process_stack_pc(f.pc);
  }
}

// The previous boot's record, copied out so the RTC record can be consumed.
inline Record prev;
inline bool prev_was_wdt = false;

// Call once from on_boot: copies the record and consumes it so it is reported once.
inline void load_previous() {
  prev = record;
  record.magic = 0;
  prev.component[sizeof(prev.component) - 1] = '\0';
  prev_was_wdt = esp_reset_reason() == ESP_RST_TASK_WDT;
}

inline const char *unavailable() {
  if (!prev_was_wdt) {
    return "n/a";
  }
  return prev.magic != RECORD_MAGIC ? "unknown (no record)" : nullptr;
}

inline void append_frames(std::string &s, const Trace &t, size_t limit) {
  char buf[16];
  for (size_t i = 0; i < t.depth && i < limit; i++) {
    snprintf(buf, sizeof(buf), " 0x%08X", (unsigned) t.pc[i]);
    s += buf;
  }
}

// Why a trace is empty.
inline const char *trace_problem(uint8_t kind) {
  switch (kind) {
    case TRACE_NO_HANDLE:
      return "task not found";
    case TRACE_NO_SNAPSHOT:
      return "TCB unreadable";
    case TRACE_BAD_STACK:
      return "bad stack pointer";
    default:
      return "no frames";
  }
}

inline void clamp(std::string &s) {
  if (s.size() > MAX_STATE_LEN) {
    s.resize(MAX_STATE_LEN - 4);
    s += " ...";
  }
}

// "<component> (uptime Ns)" after a task-watchdog reset.
inline std::string last_report() {
  if (const char *none = unavailable()) {
    return none;
  }
  char buf[64];
  snprintf(buf, sizeof(buf), "%s (uptime %us)", prev.component, (unsigned) prev.uptime_s);
  return std::string(buf);
}

// "elf=<id> stack: 0x... 0x..." : where loopTask was waiting, top frame first.
inline std::string loop_trace() {
  if (const char *none = unavailable()) {
    return none;
  }
  const Trace &t = prev.task[0];
  if (t.depth == 0) {
    return std::string("no loopTask stack (") + trace_problem(t.kind) + ")";
  }
  char elf[9] = "";
  esp_app_get_elf_sha256(elf, sizeof(elf));
  std::string s = std::string("elf=") + elf + " stack:";
  append_frames(s, t, LOOP_FRAMES);
  clamp(s);
  return s;
}

// "BTC_TASK: 0x... | BTU_TASK: 0x..." for tasks first..last (indices into TASK_NAMES): where
// those Bluetooth tasks were waiting. Two sensors are needed: six frames each for all four
// tasks would not fit in a 255-character Home Assistant state.
inline std::string task_traces(size_t first, size_t last) {
  if (const char *none = unavailable()) {
    return none;
  }
  std::string s;
  for (size_t i = first; i <= last && i < N_TASKS; i++) {
    const Trace &t = prev.task[i];
    if (!s.empty()) {
      s += " | ";
    }
    s += TASK_NAMES[i];
    s += ":";
    if (t.depth == 0) {
      s += " none (";  // say why, instead of leaving an empty sensor to guess from
      s += trace_problem(t.kind);
      s += ")";
    } else {
      append_frames(s, t, BT_FRAMES);
    }
  }
  clamp(s);
  return s;
}

}  // namespace jkbms_wdt

// Runs in interrupt context: no logging, heap allocation or locks here.
extern "C" void esp_task_wdt_isr_user_handler(void) {
  esphome::Component *component = esphome::App.get_current_component();
  const char *name = component != nullptr ? LOG_STR_ARG(component->get_component_log_str()) : "none";
  jkbms_wdt::Record &r = jkbms_wdt::record;
  r.magic = 0;  // invalid until fully written
  size_t i = 0;
  for (; i < sizeof(r.component) - 1 && name[i] != '\0'; i++) {
    r.component[i] = name[i];
  }
  r.component[i] = '\0';
  r.uptime_s = esphome::millis() / 1000;
  for (size_t n = 0; n < jkbms_wdt::N_TASKS; n++) {
    jkbms_wdt::capture_task(r.task[n], jkbms_wdt::handles[n],
                            n == 0 ? jkbms_wdt::LOOP_FRAMES : jkbms_wdt::BT_FRAMES);
  }
  r.magic = jkbms_wdt::RECORD_MAGIC;
}
#endif  // USE_ESP32
