#pragma once

// Decides whether HID++ button routing may be used for a device.
// Only Logitech pointing devices connected directly over Bluetooth Low Energy, whose report descriptor declares
// HID++ long reports, are supported. Receivers and other transports are not supported.

#include "session.hpp"
#include "types/device_identifiers.hpp"
#include <cstdint>
#include <optional>
#include <pqrs/hid.hpp>
#include <span>
#include <string>
#include <string_view>

namespace krbn::logitech::hidpp {
class device_support final {
public:
  device_support()
      : unsupported_reason_("the device is not checked") {
  }

  device_support(const device_identifiers& identifiers,
                 std::string_view transport,
                 std::span<const uint8_t> report_descriptor) {
    unsupported_reason_ = find_unsupported_device_reason(identifiers, transport);
    if (unsupported_reason_) {
      return;
    }

    auto parse_result = pqrs::hid::report_descriptor::parse(report_descriptor);
    if (!parse_result) {
      unsupported_reason_ = "the report descriptor could not be parsed";
      return;
    }

    const auto& descriptor = parse_result->get_descriptor();

    if (!declares_long_report(descriptor, pqrs::hid::report_descriptor::report_type::input) ||
        !declares_long_report(descriptor, pqrs::hid::report_descriptor::report_type::output)) {
      unsupported_reason_ = "the report descriptor does not declare HID++ long reports (report ID 0x11, 19 bytes)";
      return;
    }
  }

  // Checks which do not need the report descriptor.
  [[nodiscard]] static std::optional<std::string> find_unsupported_device_reason(const device_identifiers& identifiers,
                                                                                 std::string_view transport) {
    if (identifiers.get_vendor_id() != pqrs::hid::vendor_id::value_t(0x046d)) {
      return "only Logitech devices are supported";
    }

    if (!identifiers.get_is_pointing_device()) {
      return "only pointing devices are supported";
    }

    // macOS reports "Bluetooth Low Energy" while IOHIDKeys.h defines "BluetoothLowEnergy".
    if (transport != "Bluetooth Low Energy" &&
        transport != "BluetoothLowEnergy") {
      return fmt::format("transport `{0}` is not supported (only direct Bluetooth Low Energy connections are supported; receivers are not supported)",
                         transport);
    }

    return std::nullopt;
  }

  [[nodiscard]] const std::optional<std::string>& get_unsupported_reason() const {
    return unsupported_reason_;
  }

private:
  static bool is_vendor_defined(pqrs::hid::usage_page::value_t usage_page) {
    return pqrs::hid::usage_page::value_t(0xff00) <= usage_page &&
           usage_page <= pqrs::hid::usage_page::value_t(0xffff);
  }

  static bool declares_long_report(const pqrs::hid::report_descriptor::descriptor& descriptor,
                                   pqrs::hid::report_descriptor::report_type report_type) {
    uint64_t bits = 0;
    auto fields = descriptor.find_report_fields(report_type,
                                                pqrs::hid::report_id::value_t(long_report_id));
    for (const auto& f : fields) {
      if (!is_vendor_defined(f->get_usage_page())) {
        return false;
      }
      bits += static_cast<uint64_t>(f->get_size_bits()) * f->get_count();
    }

    // The report ID byte is not included in the descriptor size.
    return bits == (std::tuple_size_v<long_report> - 1) * 8;
  }

  std::optional<std::string> unsupported_reason_;
};
} // namespace krbn::logitech::hidpp
