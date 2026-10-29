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
#include <utility>
#include <vector>

namespace krbn::hidpp {
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

    collect_input_buttons(descriptor);
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

  // Returns the reason why `button` cannot be used as the output button.
  // A button which the device can also report through its normal input path is refused,
  // because the pressed state of physical and generated events could not be told apart.
  [[nodiscard]] std::optional<std::string> find_pointing_button_conflict(pqrs::hid::usage::value_t button) const {
    if (unsupported_reason_) {
      return *unsupported_reason_;
    }

    if (has_unknown_input_buttons_) {
      return "the report descriptor declares button input without explicit usages, so the device buttons cannot be determined";
    }

    auto b = static_cast<int64_t>(type_safe::get(button));
    for (const auto& [minimum, maximum] : input_button_ranges_) {
      if (minimum <= b && b <= maximum) {
        return fmt::format("the device can report pointing button {0} itself ({1}); choose a pointing_button which the device does not declare",
                           b,
                           make_input_buttons_string());
      }
    }

    return std::nullopt;
  }

  // The button usages declared by the report descriptor, e.g. "button1-button16".
  [[nodiscard]] std::string make_input_buttons_string() const {
    std::string s;
    for (const auto& [minimum, maximum] : input_button_ranges_) {
      if (!s.empty()) {
        s += ", ";
      }
      if (minimum == maximum) {
        s += fmt::format("button{0}", minimum);
      } else {
        s += fmt::format("button{0}-button{1}", minimum, maximum);
      }
    }
    return s.empty() ? "no buttons declared" : "declared: " + s;
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

  void add_usage_range(const std::optional<pqrs::hid::usage_pair>& minimum,
                       const std::optional<pqrs::hid::usage_pair>& maximum) {
    if (!minimum && !maximum) {
      return;
    }

    auto on_button_page = [](const auto& u) {
      return u && u->get_usage_page() == pqrs::hid::usage_page::button;
    };

    if (!on_button_page(minimum) && !on_button_page(maximum)) {
      return;
    }

    if (!minimum || !maximum || !on_button_page(minimum) || !on_button_page(maximum)) {
      // A range which crosses usage pages cannot be resolved safely.
      has_unknown_input_buttons_ = true;
      return;
    }

    input_button_ranges_.emplace_back(type_safe::get(minimum->get_usage()),
                                      type_safe::get(maximum->get_usage()));
  }

  void add_usages(const std::vector<pqrs::hid::usage_pair>& usages) {
    for (const auto& u : usages) {
      if (u.get_usage_page() == pqrs::hid::usage_page::button) {
        auto v = type_safe::get(u.get_usage());
        input_button_ranges_.emplace_back(v, v);
      }
    }
  }

  void collect_input_buttons(const pqrs::hid::report_descriptor::descriptor& descriptor) {
    using flag = pqrs::hid::report_descriptor::report_field_flag;

    for (const auto& f : descriptor.get_report_fields()) {
      if (f.get_report_type() != pqrs::hid::report_descriptor::report_type::input ||
          f.has_flag(flag::constant)) {
        continue;
      }

      add_usages(f.get_usages());
      add_usage_range(f.get_usage_minimum(), f.get_usage_maximum());
      for (const auto& set : f.get_usage_sets()) {
        add_usages(set.get_usages());
        add_usage_range(set.get_usage_minimum(), set.get_usage_maximum());
      }

      if (f.get_usage_page() == pqrs::hid::usage_page::button &&
          f.get_usages().empty() &&
          f.get_usage_sets().empty() &&
          !f.get_usage_minimum() &&
          !f.get_usage_maximum()) {
        has_unknown_input_buttons_ = true;
      }
    }

    std::ranges::sort(input_button_ranges_);
    std::vector<std::pair<int64_t, int64_t>> merged;
    for (const auto& r : input_button_ranges_) {
      if (!merged.empty() && r.first <= merged.back().second + 1) {
        merged.back().second = std::max(merged.back().second, r.second);
      } else {
        merged.push_back(r);
      }
    }
    input_button_ranges_ = std::move(merged);
  }

  std::optional<std::string> unsupported_reason_;
  std::vector<std::pair<int64_t, int64_t>> input_button_ranges_;
  bool has_unknown_input_buttons_ = false;
};
} // namespace krbn::hidpp
