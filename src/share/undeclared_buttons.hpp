#pragma once

// Some ELECOM trackballs drive buttons in bits which their HID report descriptor
// declares as constant padding. macOS creates no IOHIDElement for those bits, so they
// have to be recovered from the raw input report.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <pqrs/hid.hpp>
#include <span>
#include <vector>

namespace krbn::undeclared_buttons {
struct configuration final {
  uint8_t report_id;
  size_t buttons_bit_offset;
  size_t button_count;
  uint8_t first_button;
};

namespace details {
[[nodiscard]] inline bool same_collection_path(
    const pqrs::hid::report_descriptor::report_field& a,
    const pqrs::hid::report_descriptor::report_field& b) noexcept {
  const auto& a_path = a.get_collection_path();
  const auto& b_path = b.get_collection_path();

  if (a_path.size() != b_path.size()) {
    return false;
  }

  for (size_t i = 0; i < a_path.size(); ++i) {
    if (a_path[i].get_type() != b_path[i].get_type() ||
        a_path[i].get_usage() != b_path[i].get_usage()) {
      return false;
    }
  }

  return true;
}

[[nodiscard]] inline bool in_mouse_pointer_collection(
    const pqrs::hid::report_descriptor::report_field& field) noexcept {
  const auto& path = field.get_collection_path();

  if (path.size() < 2 ||
      !path[0].get_usage() ||
      !path[1].get_usage()) {
    return false;
  }

  return path[0].get_usage()->get_usage_page() == pqrs::hid::usage_page::generic_desktop &&
         path[0].get_usage()->get_usage() == pqrs::hid::usage::generic_desktop::mouse &&
         path[1].get_usage()->get_usage_page() == pqrs::hid::usage_page::generic_desktop &&
         path[1].get_usage()->get_usage() == pqrs::hid::usage::generic_desktop::pointer;
}
} // namespace details

// Return the raw-report layout only when both the device id and the descriptor's
// semantic structure match the known quirk. A changed descriptor must fall back to
// macOS rather than risk emitting duplicate or incorrectly numbered buttons.
[[nodiscard]] inline std::optional<configuration> find_configuration(
    pqrs::hid::vendor_id::value_t vendor_id,
    pqrs::hid::product_id::value_t product_id,
    std::span<const uint8_t> report_descriptor) {
  if (vendor_id != pqrs::hid::vendor_id::value_t(0x056e) ||    // Elecom Co., Ltd.
      (product_id != pqrs::hid::product_id::value_t(0x00fe) && // M-DT1URBK or M-DT2URBK DEFT Optical TrackBall
       product_id != pqrs::hid::product_id::value_t(0x01aa) && // M-HT1MRBK
       product_id != pqrs::hid::product_id::value_t(0x01ab) && // M-HT1MRBK
       product_id != pqrs::hid::product_id::value_t(0x01ac)    // M-HT1MRBK
       )) {
    return std::nullopt;
  }

  auto parse_result = pqrs::hid::report_descriptor::parse(report_descriptor);
  if (!parse_result) {
    return std::nullopt;
  }

  const auto& fields = parse_result->get_descriptor().get_report_fields();
  using flag = pqrs::hid::report_descriptor::report_field_flag;

  for (size_t i = 0; i + 1 < fields.size(); ++i) {
    const auto& buttons = fields[i];
    const auto& padding = fields[i + 1];

    // The normal button field declares buttons 1-5. The affected firmware drives
    // additional buttons in the otherwise unused padding bits immediately after it.
    if (buttons.get_report_type() != pqrs::hid::report_descriptor::report_type::input ||
        buttons.has_flag(flag::constant) ||
        !buttons.has_flag(flag::variable) ||
        buttons.has_flag(flag::relative) ||
        buttons.get_usage_page() != pqrs::hid::usage_page::button ||
        buttons.get_size_bits() != 1 ||
        buttons.get_count() != 5 ||
        buttons.get_logical_minimum() != 0 ||
        buttons.get_logical_maximum() != 1 ||
        !buttons.get_usage_minimum() ||
        !buttons.get_usage_maximum() ||
        buttons.get_usage_minimum()->get_usage_page() != pqrs::hid::usage_page::button ||
        buttons.get_usage_minimum()->get_usage() != pqrs::hid::usage::value_t(1) ||
        buttons.get_usage_maximum()->get_usage_page() != pqrs::hid::usage_page::button ||
        buttons.get_usage_maximum()->get_usage() != pqrs::hid::usage::value_t(5) ||
        buttons.get_bit_offset() % 8 != 0 ||
        !details::in_mouse_pointer_collection(buttons)) {
      continue;
    }

    if (padding.get_report_type() != buttons.get_report_type() ||
        padding.get_report_id() != buttons.get_report_id() ||
        !padding.has_flag(flag::constant) ||
        padding.get_bit_offset() != buttons.get_bit_offset() + 5 ||
        !padding.get_usages().empty() ||
        !padding.get_usage_sets().empty() ||
        padding.get_usage_minimum() ||
        padding.get_usage_maximum() ||
        !details::same_collection_path(buttons, padding)) {
      continue;
    }

    auto report_id = type_safe::get(buttons.get_report_id());
    auto padding_size_bits = static_cast<uint64_t>(padding.get_size_bits());
    auto padding_count = static_cast<uint64_t>(padding.get_count());
    if (padding_count != 0 &&
        padding_size_bits > std::numeric_limits<uint64_t>::max() / padding_count) {
      continue;
    }
    auto padding_bit_count = padding_size_bits * padding_count;
    auto first_button = static_cast<uint64_t>(
                            type_safe::get(buttons.get_usage_maximum()->get_usage())) +
                        1;
    if (report_id < 0 ||
        report_id > 255 ||
        padding_bit_count == 0 ||
        padding_bit_count > std::numeric_limits<size_t>::max() ||
        first_button > std::numeric_limits<uint8_t>::max() ||
        padding_bit_count > std::numeric_limits<uint8_t>::max() - first_button + 1) {
      continue;
    }

    // IOHIDDeviceRegisterInputReportCallback includes a leading report ID byte
    // when the descriptor uses report IDs.
    auto buttons_bit_offset = padding.get_bit_offset();
    if (report_id != 0) {
      if (buttons_bit_offset > std::numeric_limits<size_t>::max() - 8) {
        continue;
      }
      buttons_bit_offset += 8;
    }

    auto button_count = static_cast<size_t>(padding_bit_count);
    if (button_count > std::numeric_limits<size_t>::max() - buttons_bit_offset) {
      continue;
    }

    return configuration{
        .report_id = static_cast<uint8_t>(report_id),
        .buttons_bit_offset = buttons_bit_offset,
        .button_count = button_count,
        .first_button = static_cast<uint8_t>(first_button),
    };
  }

  return std::nullopt;
}

struct button_change final {
  uint8_t button;
  bool pressed;

  bool operator==(const button_change&) const = default;
};

class decoder final {
public:
  explicit decoder(const configuration& configuration)
      : configuration_(configuration),
        last_buttons_(configuration.button_count, 0) {
  }

  [[nodiscard]] std::vector<button_change> update(uint8_t report_id,
                                                  std::span<const uint8_t> report) {
    std::vector<button_change> result;

    if (report_id != configuration_.report_id ||
        configuration_.button_count == 0 ||
        configuration_.button_count - 1 >
            std::numeric_limits<size_t>::max() - configuration_.buttons_bit_offset) {
      return result;
    }

    auto last_bit_offset = configuration_.buttons_bit_offset +
                           configuration_.button_count - 1;
    if (last_bit_offset / 8 >= report.size()) {
      return result;
    }

    for (size_t i = 0; i < configuration_.button_count; ++i) {
      auto bit_offset = configuration_.buttons_bit_offset + i;
      auto mask = static_cast<uint8_t>(1u << (bit_offset % 8));
      auto pressed = (report[bit_offset / 8] & mask) != 0;

      if (pressed != (last_buttons_[i] != 0)) {
        result.push_back(button_change{
            .button = static_cast<uint8_t>(configuration_.first_button + i),
            .pressed = pressed,
        });
      }

      last_buttons_[i] = pressed ? 1 : 0;
    }

    return result;
  }

  void reset() {
    std::fill(last_buttons_.begin(), last_buttons_.end(), 0);
  }

private:
  configuration configuration_;
  std::vector<uint8_t> last_buttons_;
};
} // namespace krbn::undeclared_buttons
