#pragma once

#include "core_configuration/core_configuration.hpp"
#include "device_properties.hpp"

namespace krbn::device_utility {

inline bool determine_is_built_in_keyboard(const core_configuration::details::device& device,
                                           const device_properties& device_properties) {
  return device_properties.get_is_built_in_keyboard() ||
         device.get_treat_as_built_in_keyboard();
}

inline bool determine_should_ignore_device(const core_configuration::details::device& device,
                                           const device_properties& device_properties) {
  if (device_properties.get_device_identifiers().get_is_pointing_device() &&
      device_properties.get_is_apple() &&
      !device.get_ignore_configured()) {
    return true;
  }

  return device.get_ignore();
}

} // namespace krbn::device_utility
