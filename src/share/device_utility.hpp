#pragma once

#include "core_configuration/core_configuration.hpp"
#include "device_properties.hpp"

namespace krbn::device_utility {
inline bool determine_is_built_in_keyboard(const core_configuration::core_configuration& core_configuration,
                                           const device_properties& device_properties) {
  if (device_properties.get_is_built_in_keyboard()) {
    return true;
  }

  auto d = core_configuration.get_selected_profile().get_device(
      device_properties.get_device_identifiers());

  return d->get_treat_as_built_in_keyboard();
}

inline bool determine_should_ignore_device(const core_configuration::core_configuration& core_configuration,
                                           const device_properties& device_properties) {
  const auto& identifiers = device_properties.get_device_identifiers();
  auto d = core_configuration.get_selected_profile().get_device(identifiers);

  if (identifiers.get_is_pointing_device() &&
      device_properties.get_is_apple() &&
      !d->get_ignore_configured()) {
    return true;
  }

  return d->get_ignore();
}
} // namespace krbn::device_utility
