#pragma once

#include "libkrbn/libkrbn.h"
#include "monitor/configuration_monitor.hpp"
#include <pqrs/thread_wait.hpp>

class libkrbn_cpp final {
public:
  static krbn::device_identifiers make_device_identifiers(const libkrbn_device_identifiers& device_identifiers) {
    krbn::device_identifiers identifiers(pqrs::hid::vendor_id::value_t(device_identifiers.vendor_id),
                                         pqrs::hid::product_id::value_t(device_identifiers.product_id),
                                         device_identifiers.is_keyboard,
                                         device_identifiers.is_pointing_device);
    return identifiers;
  }
};
