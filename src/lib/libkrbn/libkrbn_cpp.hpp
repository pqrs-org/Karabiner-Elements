#pragma once

#include "libkrbn.h"

class libkrbn_cpp final {
public:
  static krbn::core_configuration::profile::device::identifiers make_device_identifiers(const libkrbn_device_identifiers& device_identifiers) {
    krbn::core_configuration::profile::device::identifiers identifiers(krbn::vendor_id(device_identifiers.vendor_id),
                                                                       krbn::product_id(device_identifiers.product_id),
                                                                       device_identifiers.is_keyboard,
                                                                       device_identifiers.is_pointing_device);
    return identifiers;
  }
};
