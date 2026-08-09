#pragma once

#include "hid_report_only_events/report_handler.hpp"
#include "hid_report_only_events/elecom/trackball.hpp"
#include "types/device_identifiers.hpp"
#include <memory>
#include <span>

namespace krbn::hid_report_only_events {
// Keep device-specific selection in this registry so the IOKit monitor remains
// independent of individual vendors and device families.
[[nodiscard]] inline bool is_target_device(
    const device_identifiers& identifiers) noexcept {
  if (identifiers.get_is_pointing_device() &&
      elecom::trackball::is_target_device(identifiers.get_vendor_id(),
                                          identifiers.get_product_id())) {
    return true;
  }

  return false;
}

[[nodiscard]] inline std::shared_ptr<report_handler> make_report_handler(
    const device_identifiers& identifiers,
    std::span<const uint8_t> report_descriptor) {
  if (identifiers.get_is_pointing_device()) {
    if (auto handler = elecom::trackball::make_report_handler(
            identifiers.get_vendor_id(),
            identifiers.get_product_id(),
            report_descriptor)) {
      return handler;
    }
  }

  return nullptr;
}
} // namespace krbn::hid_report_only_events
