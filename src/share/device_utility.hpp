#pragma once

#include "core_configuration/core_configuration.hpp"
#include "device_properties.hpp"

namespace krbn {
namespace device_utility {
inline bool determine_is_built_in_keyboard(const core_configuration::core_configuration& core_configuration,
                                           const device_properties& device_properties) {
  if (device_properties.get_is_built_in_keyboard()) {
    return true;
  }

  return core_configuration.get_selected_profile().get_device_treat_as_built_in_keyboard(
      device_properties.get_device_identifiers());
}
} // namespace device_utility
} // namespace krbn
