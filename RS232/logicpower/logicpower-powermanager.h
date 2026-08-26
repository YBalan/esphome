#pragma once

// Minimal RS232 comport monitor for the LogicPower LPM-PSW UPS "PowerManager" service port.
//
// This intentionally does no protocol decoding. It just logs and publishes every RX/TX frame
// as raw hex/ASCII, and lets you send arbitrary bytes from Home Assistant, so the wire can be
// inspected and driven manually (e.g. while probing commands by hand, or watching what another
// tool such as the vendor PowerManager app puts on the line).

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>

namespace logicpower_powermanager {

static const char *const TAG = "logicpower_powermanager";

// ---------------------------------------------------------------------------------------------
// byte / string helpers
// ---------------------------------------------------------------------------------------------

inline std::string to_hex(const std::vector<uint8_t> &bytes) {
  std::string out;
  char buf[4];
  for (size_t i = 0; i < bytes.size(); i++) {
    std::snprintf(buf, sizeof(buf), "%02X", bytes[i]);
    if (i)
      out += ' ';
    out += buf;
  }
  return out;
}

inline std::string to_printable(const std::vector<uint8_t> &bytes) {
  std::string out;
  out.reserve(bytes.size());
  for (uint8_t b : bytes) {
    out += (b >= 0x20 && b < 0x7F) ? static_cast<char>(b) : '.';
  }
  return out;
}

// ---------------------------------------------------------------------------------------------
// Voltronic PI CRC-16 - kept only as an opt-in helper for the raw command field (crc: prefix),
// for anyone who wants to hand-test a CRC-framed command without computing the bytes themselves.
// Table-driven CRC-16/CCITT variant with delimiter-byte escaping, verified byte-for-byte against
// the known-good QPI/QMOD/QPIGS/QPIRI/QPIWS command bytes in logicpower-protocol-test.yaml.
// ---------------------------------------------------------------------------------------------

inline void crc16_pi(const std::vector<uint8_t> &data, uint8_t &high, uint8_t &low) {
  static const uint16_t table[16] = {0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
                                      0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF};
  uint16_t crc = 0;
  for (uint8_t c : data) {
    uint8_t da = ((crc >> 8) & 0xFF) >> 4;
    crc = (crc << 4) & 0xFFFF;
    crc ^= table[da ^ (c >> 4)];

    da = ((crc >> 8) & 0xFF) >> 4;
    crc = (crc << 4) & 0xFFFF;
    crc ^= table[da ^ (c & 0x0F)];
  }
  low = crc & 0xFF;
  high = (crc >> 8) & 0xFF;
  if (low == 0x28 || low == 0x0D || low == 0x0A || low == 0x00)
    low++;
  if (high == 0x28 || high == 0x0D || high == 0x0A || high == 0x00)
    high++;
}

// ---------------------------------------------------------------------------------------------
// transmit
// ---------------------------------------------------------------------------------------------

inline void send_raw(const std::vector<uint8_t> &bytes) {
  if (bytes.empty())
    return;
  id(ups_uart).write_array(bytes.data(), bytes.size());
}

inline void send_ascii(const std::string &command) {
  send_raw(std::vector<uint8_t>(command.begin(), command.end()));
}

inline void send_ascii_crc(const std::string &command) {
  std::vector<uint8_t> bytes(command.begin(), command.end());
  uint8_t high, low;
  crc16_pi(bytes, high, low);
  bytes.push_back(high);
  bytes.push_back(low);
  bytes.push_back('\r');
  send_raw(bytes);
}

// Accepts free-form text typed into the Home Assistant "Raw Command" field:
//   QPIGS            -> sent verbatim, exactly as typed, no bytes added
//   crc:QPIGS        -> sent as an ASCII command with a computed PI30 CRC-16 + CR appended
//   hex:51 50 49 0D  -> sent as the exact raw bytes, space/comma separated hex pairs
inline void send_from_text_field(const std::string &text) {
  if (text.rfind("hex:", 0) == 0) {
    std::vector<uint8_t> bytes;
    std::string token;
    for (size_t i = 4; i <= text.size(); i++) {
      if (i == text.size() || text[i] == ' ' || text[i] == ',') {
        if (!token.empty()) {
          bytes.push_back(static_cast<uint8_t>(std::strtoul(token.c_str(), nullptr, 16)));
          token.clear();
        }
      } else {
        token += text[i];
      }
    }
    send_raw(bytes);
    return;
  }
  if (text.rfind("crc:", 0) == 0) {
    send_ascii_crc(text.substr(4));
    return;
  }
  send_ascii(text);
}

// ---------------------------------------------------------------------------------------------
// receive - raw traffic visibility only, no parsing
// ---------------------------------------------------------------------------------------------

inline void on_uart_frame(esphome::uart::UARTDirection direction, std::vector<uint8_t> &bytes) {
  const std::string hex = to_hex(bytes);
  const std::string ascii = to_printable(bytes);
  if (direction == esphome::uart::UART_DIRECTION_TX) {
    id(ups_last_tx_raw).publish_state(hex);
    id(ups_last_tx_ascii).publish_state(ascii);
    ESP_LOGI(TAG, "TX %s | %s", hex.c_str(), ascii.c_str());
    return;
  }
  id(ups_last_rx_raw).publish_state(hex);
  id(ups_last_rx_ascii).publish_state(ascii);
  ESP_LOGI(TAG, "RX %s | %s", hex.c_str(), ascii.c_str());
}

}  // namespace logicpower_powermanager
