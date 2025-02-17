#pragma once

#include "libkrbn/libkrbn.h"
#include "monitor/configuration_monitor.hpp"
#include <pqrs/thread_wait.hpp>

class libkrbn_components_manager;
extern std::shared_ptr<libkrbn_components_manager> libkrbn_components_manager_;

class libkrbn_cpp final {
public:
  static std::shared_ptr<libkrbn_components_manager> get_components_manager(void) {
    return libkrbn_components_manager_;
  }

  static krbn::device_identifiers make_device_identifiers(const libkrbn_device_identifiers* device_identifiers) {
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
