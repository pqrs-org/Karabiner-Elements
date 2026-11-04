#pragma once

#include "core_configuration/details/profile/logitech_hidpp.hpp"
#include "manipulator/types/to_event_definition.hpp"

namespace krbn::core_service::daemon::device_grabber_details::logitech_hidpp_configuration_utility {
[[nodiscard]] inline std::vector<uint16_t> make_validated_enabled_control_ids(
    const std::optional<core_configuration::details::logitech_hidpp>& configuration) {
  std::vector<uint16_t> result;

  if (configuration) {
    for (const auto& button : configuration->get_buttons()) {
      if (!button.enabled) {
        continue;
      }

      for (const auto& value : button.to) {
        [[maybe_unused]] const manipulator::to_event_definition definition(value);
      }
      result.push_back(button.control_id);
    }
  }

  return result;
}
} // namespace krbn::core_service::daemon::device_grabber_details::logitech_hidpp_configuration_utility
