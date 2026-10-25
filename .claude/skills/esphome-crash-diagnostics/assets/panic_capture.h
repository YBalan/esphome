#pragma once

#ifdef USE_ESP32
#include <cstring>
#include <string>
#include <esp_app_desc.h>
#ifdef USE_ESP32_CRASH_HANDLER
#include "esphome/components/esp32/crash_handler.h"
#endif

// Remote panic report for devices with no serial access.
//
// The panic text goes only to UART0, so an "exception/panic" reset leaves no
// trace. Since ESPHome 2026.3 the esp32 component ships its own crash handler:
// it wraps esp_panic_handler() (link flag -Wl,--wrap=esp_panic_handler, added by
// ESPHome itself), keeps the crashed core's PC, fault address and backtraces in
// no-init RAM across the reboot, and logs them with tag "esp32.crash" at boot and
// to the first API client that connects. This file must NOT define
// __wrap_esp_panic_handler any more: two definitions fail at link time.
//
// What is left to do here is turn that log output into short strings for text
// sensors (Home Assistant drops states longer than 255 characters), so the
// report is also kept in HA history. load_previous() replays the stored crash
// through the logger and on_log() (fed by `logger: on_message`, see the YAML)
// picks the lines up. Calling crash_handler_log() does not consume the record,
// so a connecting API client still gets the full log.
//
// The backtrace is a list of code addresses. Turn them into function names with
// the ELF of the firmware that was running (keep a copy of firmware.elf for
// every build you flash), e.g. with tools/decode_panic.py. `esphome logs`
// decodes the same lines automatically when it is connected during the boot.
namespace panic_capture {

constexpr size_t MAX_STATE_LEN = 250;  // HA drops states longer than 255
constexpr const char *CRASH_TAG = "esp32.crash";

inline std::string g_summary = "n/a";
inline std::string g_backtrace = "n/a";

// Collected from the replayed log lines (only while load_previous() runs).
inline bool g_collecting = false;
inline bool g_other_core = false;
inline std::string g_report;
inline std::string g_bt_crashed;
inline std::string g_bt_other;

inline bool starts_with(const char *s, const char *prefix) { return strncmp(s, prefix, strlen(prefix)) == 0; }

inline void append_part(std::string &dst, const char *text, const char *sep) {
  if (!dst.empty()) {
    dst += sep;
  }
  dst += text;
}

// Feed every log line here (logger: on_message). Only the crash handler's
// lines are used, and only while load_previous() is replaying them.
inline void on_log(const char *tag, const char *message) {
  if (!g_collecting || tag == nullptr || message == nullptr || strcmp(tag, CRASH_TAG) != 0) {
    return;
  }
  while (*message == ' ') {
    message++;
  }
  // "Reason: Fault - IllegalInstruction (cause 0)", "Crashed core: 1",
  // "PC:  0x400D1234  (fault location)", "EXCVADDR: 0x00000000  (faulting address)".
  // Lowercase pc:/excvaddr: are the same fields when another build captured them.
  if (starts_with(message, "Reason:") || starts_with(message, "Crashed core:") || starts_with(message, "PC:") ||
      starts_with(message, "pc:") || starts_with(message, "EXCVADDR:") || starts_with(message, "excvaddr:")) {
    append_part(g_report, message, " | ");
  } else if (starts_with(message, "Captured by a different")) {
    append_part(g_report, "other-build (addresses belong to that ELF)", " | ");
  } else if (starts_with(message, "Other core (") || starts_with(message, "other core (")) {
    g_other_core = true;  // the BT lines that follow are the second core's stack
  } else if ((starts_with(message, "BT") || starts_with(message, "bt")) && message[2] >= '0' && message[2] <= '9') {
    // "BT3: 0x400D1234  (backtrace)" -> keep only the address.
    const char *addr = strstr(message, "0x");
    if (addr != nullptr && strlen(addr) >= 10) {
      append_part(g_other_core ? g_bt_other : g_bt_crashed, std::string(addr, 10).c_str(), " ");
    }
  }
}

inline void clamp(std::string &s) {
  if (s.size() > MAX_STATE_LEN) {
    s.resize(MAX_STATE_LEN - 4);
    s += " ...";
  }
}

// Describe the previous crash (if any). Call from on_boot, then read summary()
// and backtrace().
inline void load_previous() {
  g_summary = "n/a";
  g_backtrace = "n/a";
  g_report.clear();
  g_bt_crashed.clear();
  g_bt_other.clear();
  g_other_core = false;
#ifdef USE_ESP32_CRASH_HANDLER
  if (!esphome::esp32::crash_handler_has_data()) {
    return;
  }
  g_collecting = true;
  esphome::esp32::crash_handler_log();
  g_collecting = false;
#else
  return;
#endif

  if (g_report.empty() && g_bt_crashed.empty()) {
    g_report = "crash data present but not readable";
  }
  // The ELF id of the running build: it must match the ELF you decode with.
  char elf[9] = "";
  esp_app_get_elf_sha256(elf, sizeof(elf));
  g_report += " | elf=";
  g_report += elf;
  clamp(g_report);
  g_summary = g_report;

  std::string bt = g_bt_crashed;
  if (!g_bt_other.empty()) {
    bt += bt.empty() ? "other core: " : " | other core: ";
    bt += g_bt_other;
  }
  clamp(bt);
  g_backtrace = bt.empty() ? "n/a" : bt;
}

inline const std::string &summary() { return g_summary; }
inline const std::string &backtrace() { return g_backtrace; }

}  // namespace panic_capture
#endif  // USE_ESP32
