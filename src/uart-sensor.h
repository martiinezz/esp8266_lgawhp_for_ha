#pragma once

#include "esphome.h"

namespace esphome {

class UARTHexSensor : public Component, public uart::UARTDevice {
 private:
  static const size_t PACKET_SIZE = 20;
  uint32_t last_read_time_{0};
  uint8_t last_packet_[PACKET_SIZE]{0};  // Store the last received packet
  bool has_last_packet_{false};          // Flag to track if we have a previous packet
  static const uint32_t DEDUP_WINDOW_MS = 100;  // Time window to consider packets as duplicates

  const char* get_operation_mode_(uint8_t status_byte) {
    if (status_byte & 0x80) {
      // Thermostat mode active
      if (status_byte & 0x10) return "THERMO-HEAT";
      if (status_byte & 0x20) return "THERMO-COOL";
      return "THERMOSTAT_UNKNOWN";
    } else {
      // Normal modes
      if (status_byte & 0x08) return "AI";
      if (status_byte & 0x10) return "HEAT";
      if (status_byte & 0x20) return "COOL";
      return "UNKNOWN";
    }
  }

  // Helper method to get outdoor unit status
  const char* get_outdoor_unit_status_(uint8_t status_byte) {
    switch (status_byte) {
      case 0x06: return "DFST";     // Defrost mode
      case 0x04: return "VENT";     // Venting mode
      case 0x02: return "ON";       // Normal operation
      default: return "OFF";        // Unit is off
    }
  }

  // Helper method to check if the packet is a duplicate
  bool is_duplicate_packet_(const uint8_t* data) {
    // If we haven't received a packet before, this can't be a duplicate
    if (!has_last_packet_) {
      return false;
    }

    // Check if we're within the deduplication time window
    if ((millis() - last_read_time_) > DEDUP_WINDOW_MS) {
      return false;  // Enough time has passed, treat as new packet
    }

    // Compare current packet with the last one
    return memcmp(data, last_packet_, PACKET_SIZE) == 0;
  }

  // Helper method to store the packet for duplicate detection
  void store_packet_(const uint8_t* data) {
    memcpy(last_packet_, data, PACKET_SIZE);
    has_last_packet_ = true;
  }

  // Method to log and interpret the received packet
  void log_and_interpret_packet_(const uint8_t* data, size_t len) {
    // Only process if we have a full packet
    if (len != PACKET_SIZE) {
      return;
    }

    // Check for duplicate packets
    if (data[0] == 0xC0 && is_duplicate_packet_(data)) {
      ESP_LOGV("UART", "Skipping duplicate packet");  // Verbose log for debugging
      return;
    }

    // Store this packet for future duplicate detection
    store_packet_(data);

    // First log the raw packet
    char hex[len * 3 + 1];
    size_t hex_index = 0;

    for (size_t i = 0; i < len; i++) {
      hex_index += snprintf(&hex[hex_index], sizeof(hex) - hex_index, "%02X", data[i]);
      if (i < len - 1 && hex_index < sizeof(hex) - 1) {
        hex[hex_index++] = ' ';
      }
    }
    hex[hex_index] = '\0';

    ESP_LOGI("UART", "Received packet (%d bytes): %s", len, hex);

    if (data[0] == 0xC0) {
      // Power state
      bool power_state = (data[1] & 0x02) != 0;
      const char* power_state_str = power_state ? "ON" : "OFF";
      const char* operation_mode_str = get_operation_mode_(data[1]);

      ESP_LOGI("UART", "Power: %s       Mode: %s", power_state_str, operation_mode_str);

      // Publish to HA
      if (this->power_state_sensor != nullptr)
        this->power_state_sensor->publish_state(power_state_str);
      if (this->operation_mode_sensor != nullptr)
        this->operation_mode_sensor->publish_state(operation_mode_str);

      // Component states in one line
      const char* outdoor_unit_status = get_outdoor_unit_status_(data[3]);
      bool water_pump = (data[2] & 0x02);
      bool silent_mode = (data[3] & 0x08);

      ESP_LOGI("UART", "OutDoorUnit=%s  WaterPump=%s  Silent=%s",
        outdoor_unit_status,
        water_pump ? "ON" : "OFF",
        silent_mode ? "ON" : "OFF");

      // Publish to HA
      if (this->outdoor_unit_sensor != nullptr)
        this->outdoor_unit_sensor->publish_state(outdoor_unit_status);
      if (this->water_pump_sensor != nullptr)
        this->water_pump_sensor->publish_state(water_pump ? "ON" : "OFF");
      if (this->silent_mode_sensor != nullptr)
        this->silent_mode_sensor->publish_state(silent_mode ? "ON" : "OFF");

      // Temperatures in one line
      int inlet_temp = data[11];
      int outlet_temp = data[12];

      ESP_LOGI("UART", "Water temps:     Inlet=%d°C     Outlet=%d°C", inlet_temp, outlet_temp);

      // Publish to HA
      if (this->inlet_temp_sensor != nullptr)
        this->inlet_temp_sensor->publish_state(inlet_temp);
      if (this->outlet_temp_sensor != nullptr)
        this->outlet_temp_sensor->publish_state(outlet_temp);

      // Indoor temperature and set temperature
      float indoor_temp = 10 + data[5] * 0.5;
      uint8_t target_temp = data[8];
      if (target_temp > 18 && target_temp < 55) {
        ESP_LOGI("UART", "Indoor: %.1f°C   Set temp=%d°C", indoor_temp, target_temp);

        // Publish to HA
        if (this->indoor_temp_sensor != nullptr)
          this->indoor_temp_sensor->publish_state(indoor_temp);
        if (this->set_temp_sensor != nullptr)
          this->set_temp_sensor->publish_state(target_temp);
      }
      // Add a separator line for better readability
      ESP_LOGI("UART", "----------------------------------------");
    }
  }

 public:
  // Constructor
  UARTHexSensor(uart::UARTComponent *parent) : uart::UARTDevice(parent) {}

  // Pointers to sensors
  text_sensor::TextSensor *power_state_sensor{nullptr};
  text_sensor::TextSensor *operation_mode_sensor{nullptr};
  text_sensor::TextSensor *outdoor_unit_sensor{nullptr};
  text_sensor::TextSensor *water_pump_sensor{nullptr};
  text_sensor::TextSensor *silent_mode_sensor{nullptr};

  sensor::Sensor *inlet_temp_sensor{nullptr};
  sensor::Sensor *outlet_temp_sensor{nullptr};
  sensor::Sensor *indoor_temp_sensor{nullptr};
  sensor::Sensor *set_temp_sensor{nullptr};

  void setup() override {
    this->check_uart_settings(300);
  }

  void loop() override {
    if (available()) {
      uint8_t data[PACKET_SIZE];
      size_t len = 0;
      uint32_t start_time = millis();

      while (len < PACKET_SIZE && (millis() - start_time) < 1000) {
        if (available()) {
          data[len++] = read();
          last_read_time_ = millis();
        } else {
          delay(10);
        }
      }

      if (len > 0) {
        log_and_interpret_packet_(data, len);
      }
    }
  }

  float get_setup_priority() const override { return setup_priority::DATA; }
};

}  // namespace esphome
