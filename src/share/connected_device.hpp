#pragma once

#include "connected_device_capabilities.hpp"
#include "device_properties.hpp"
#include <pqrs/gsl.hpp>

namespace krbn {
struct connected_device final {
  pqrs::not_null_shared_ptr_t<device_properties> properties;
  connected_device_capabilities capabilities;
};
} // namespace krbn
