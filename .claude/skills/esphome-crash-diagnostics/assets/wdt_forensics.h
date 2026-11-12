#pragma once

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
// loop_trace() and task_traces() turn it into text for the "Last Watchdog*"
// text sensors. Decode the addresses with decode_panic.py (loop_trace() starts with
// the elf= id so the right ELF is picked from elf-archive/).
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
//
// Include this file once (esphome: includes:). It defines a global C symbol, so
// including it from a second translation unit would be a duplicate definition.
namespace wdt_forensics {

constexpr uint32_t RECORD_MAGIC = 0x57444733;  // "WDG3"
constexpr size_t N_TASKS = 5;                  // loopTask + the Bluetooth host/controller tasks
constexpr size_t LOOP_FRAMES = 16;             // loopTask: the stack that shows where it waits
constexpr size_t BT_FRAMES = 6;                // Bluetooth tasks: wait primitive and who called it
constexpr size_t MAX_STATE_LEN = 250;          // Home Assistant drops states longer than 255
// Tasks to capture: loopTask first, then the ones worth seeing for this device. These are the
// Bluetooth tasks of the Bluedroid stack; for other suspects use e.g. "tiT" (lwIP), "wifi",
// "esphome_mqtt". Keep N_TASKS and the task_traces() calls in step.
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

}  // namespace wdt_forensics

// Runs in interrupt context: no logging, heap allocation or locks here.
extern "C" void esp_task_wdt_isr_user_handler(void) {
  esphome::Component *component = esphome::App.get_current_component();
  const char *name = component != nullptr ? LOG_STR_ARG(component->get_component_log_str()) : "none";
  wdt_forensics::Record &r = wdt_forensics::record;
  r.magic = 0;  // invalid until fully written
  size_t i = 0;
  for (; i < sizeof(r.component) - 1 && name[i] != '\0'; i++) {
    r.component[i] = name[i];
  }
  r.component[i] = '\0';
  r.uptime_s = esphome::millis() / 1000;
  for (size_t n = 0; n < wdt_forensics::N_TASKS; n++) {
    wdt_forensics::capture_task(r.task[n], wdt_forensics::handles[n],
                            n == 0 ? wdt_forensics::LOOP_FRAMES : wdt_forensics::BT_FRAMES);
  }
  r.magic = wdt_forensics::RECORD_MAGIC;
}
#endif  // USE_ESP32
