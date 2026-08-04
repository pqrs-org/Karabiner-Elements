#pragma once

#include "monitor/configuration_monitor.hpp"
#include "settings.hpp"
#include <pqrs/thread_wait.hpp>

class settings_components_manager;
extern std::shared_ptr<settings_components_manager> settings_components_manager_;

class settings_cpp final {
public:
  [[nodiscard]] static std::shared_ptr<settings_components_manager> get_components_manager() {
    return settings_components_manager_;
  }

  [[nodiscard]] static krbn::device_identifiers make_device_identifiers(const krbn_device_identifiers* device_identifiers) {
    if (device_identifiers) {
      return krbn::device_identifiers(pqrs::hid::vendor_id::value_t(device_identifiers->vendor_id),
                                      pqrs::hid::product_id::value_t(device_identifiers->product_id),
                                      device_identifiers->is_keyboard,
                                      device_identifiers->is_pointing_device,
                                      device_identifiers->is_game_pad,
                                      device_identifiers->is_consumer,
                                      device_identifiers->is_virtual_device,
                                      device_identifiers->device_address);
    }

    return krbn::device_identifiers();
  }
};
