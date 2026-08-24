#pragma once

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace logicpower_megatec {

static const char *const TAG = "logicpower_megatec";
static uint32_t last_status_ms = 0;

inline std::string trim_response(const std::vector<uint8_t> &bytes) {
  std::string response(bytes.begin(), bytes.end());
  while (!response.empty() && (response.back() == '\r' || response.back() == '\n' || response.back() == '\0')) {
    response.pop_back();
  }
  return response;
}

inline bool is_connected() {
  return last_status_ms != 0 && millis() - last_status_ms < 15000;
}

inline void publish_status(const char *flags) {
  const bool utility_fail = flags[0] == '1';
  const bool battery_low = flags[1] == '1';
  const bool bypass_active = flags[2] == '1';
  const bool failed = flags[3] == '1';
  const bool standby_type = flags[4] == '1';
  const bool test_active = flags[5] == '1';
  const bool shutdown_active = flags[6] == '1';
  const bool beeper_on = flags[7] == '1';

  id(ups_utility_fail).publish_state(utility_fail);
  id(ups_battery_low).publish_state(battery_low);
  id(ups_bypass_active).publish_state(bypass_active);
  id(ups_failed).publish_state(failed);
  id(ups_standby_type).publish_state(standby_type);
  id(ups_test_active).publish_state(test_active);
  id(ups_shutdown_active).publish_state(shutdown_active);
  id(ups_beeper_on).publish_state(beeper_on);

  const char *status = "Online";
  if (failed) {
    status = "Fault";
  } else if (shutdown_active) {
    status = "Shutdown pending";
  } else if (test_active) {
    status = "Battery test";
  } else if (bypass_active) {
    status = "Bypass / AVR";
  } else if (utility_fail) {
    status = "On battery";
  }
  id(ups_status).publish_state(status);
}

inline bool parse_status(const std::string &response) {
  float input_voltage;
  float input_fault_voltage;
  float output_voltage;
  float output_load;
  float input_frequency;
  float battery_voltage;
  float temperature;
  char flags[9]{};

  const int fields = std::sscanf(response.c_str(), "(%f %f %f %f %f %f %f %8[01]", &input_voltage,
                                 &input_fault_voltage, &output_voltage, &output_load, &input_frequency,
                                 &battery_voltage, &temperature, flags);
  if (fields != 8 || std::char_traits<char>::length(flags) != 8) {
    return false;
  }

  id(ups_input_voltage).publish_state(input_voltage);
  id(ups_input_fault_voltage).publish_state(input_fault_voltage);
  id(ups_output_voltage).publish_state(output_voltage);
  id(ups_output_load).publish_state(output_load);
  id(ups_input_frequency).publish_state(input_frequency);
  id(ups_battery_voltage).publish_state(battery_voltage);
  id(ups_temperature).publish_state(temperature);
  publish_status(flags);
  last_status_ms = millis();
  return true;
}

inline bool parse_ratings(const std::string &response) {
  float output_voltage;
  float output_current;
  float battery_voltage;
  float input_frequency;
  char trailing;

  const int fields = std::sscanf(response.c_str(), "#%f %f %f %f %c", &output_voltage, &output_current,
                                 &battery_voltage, &input_frequency, &trailing);
  if (fields != 4) {
    return false;
  }

  id(ups_rated_output_voltage).publish_state(output_voltage);
  id(ups_rated_output_current).publish_state(output_current);
  id(ups_rated_battery_voltage).publish_state(battery_voltage);
  id(ups_rated_input_frequency).publish_state(input_frequency);
  id(ups_ratings).publish_state(response);
  return true;
}

inline void parse_response(esphome::uart::UARTDirection direction, std::vector<uint8_t> &bytes) {
  UARTDebug::log_string(direction, bytes);
  const std::string response = trim_response(bytes);
  if (response.empty()) {
    return;
  }

  id(ups_last_response).publish_state(response);
  if (response.front() == '(' && parse_status(response)) {
    ESP_LOGD(TAG, "Valid Q1 status: %s", response.c_str());
    return;
  }
  if (response.front() == '#' && parse_ratings(response)) {
    ESP_LOGD(TAG, "Valid F ratings: %s", response.c_str());
    return;
  }
  if (response.front() == '#') {
    id(ups_identification).publish_state(response.substr(1));
    ESP_LOGD(TAG, "Identification: %s", response.c_str());
    return;
  }
  if (response == "ACK" || response == "(ACK") {
    id(ups_last_command_result).publish_state("Accepted");
  } else {
    id(ups_last_command_result).publish_state(response);
  }
}

inline void send_command(const std::string &command, const char *description) {
  id(ups_uart).write_str(command.c_str());
  id(ups_last_command).publish_state(description);
  id(ups_last_command_result).publish_state("Sent");
  ESP_LOGI(TAG, "Sent %s", description);
}

inline std::string two_digits(int value) {
  char buffer[3];
  std::snprintf(buffer, sizeof(buffer), "%02d", std::max(0, std::min(99, value)));
  return buffer;
}

inline std::string four_digits(int value) {
  char buffer[5];
  std::snprintf(buffer, sizeof(buffer), "%04d", std::max(0, std::min(9999, value)));
  return buffer;
}

}  // namespace logicpower_megatec