#pragma once

// Some ELECOM trackballs drive buttons in bits which their HID report descriptor
// declares as constant padding. macOS creates no IOHIDElement for those bits, so they
// have to be recovered from the raw input report.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <pqrs/hid.hpp>
#include <span>
#include <vector>

namespace krbn::undeclared_buttons {
struct configuration final {
  uint8_t report_id;
  size_t buttons_byte_index;
  uint8_t button_mask;
};

// Return the raw-report layout only when both the device id and the relevant part of its
// descriptor match the known quirk. A changed descriptor must fall back to macOS rather
// than risk emitting duplicate or incorrectly numbered buttons.
[[nodiscard]] inline std::optional<configuration> find_configuration(
    pqrs::hid::vendor_id::value_t vendor_id,
    pqrs::hid::product_id::value_t product_id,
    std::span<const uint8_t> report_descriptor) {
  if (vendor_id != pqrs::hid::vendor_id::value_t(0x056e) ||
      (product_id != pqrs::hid::product_id::value_t(0x01aa) &&
       product_id != pqrs::hid::product_id::value_t(0x01ab) &&
       product_id != pqrs::hid::product_id::value_t(0x01ac))) {
    return std::nullopt;
  }

  // Mouse collection prefix from a real ELECOM HUGE PLUS (M-HT1MRBK, 056e:01aa).
  // Buttons 1-5 occupy the low five bits of report 1; the firmware drives Fn1-Fn3 in
  // the three constant bits immediately following them. Linux handles these same three
  // product ids in drivers/hid/hid-elecom.c.
  // clang-format off
  constexpr std::array<uint8_t, 34> descriptor_prefix = {
      0x05, 0x01, // Usage Page (Generic Desktop)
      0x09, 0x02, // Usage (Mouse)
      0xa1, 0x01, // Collection (Application)
      0x85, 0x01, //   Report ID (1)
      0x09, 0x01, //   Usage (Pointer)
      0xa1, 0x00, //   Collection (Physical)
      0x05, 0x09, //     Usage Page (Button)
      0x19, 0x01, //     Usage Minimum (1)
      0x29, 0x05, //     Usage Maximum (5)
      0x15, 0x00, //     Logical Minimum (0)
      0x25, 0x01, //     Logical Maximum (1)
      0x75, 0x01, //     Report Size (1)
      0x95, 0x05, //     Report Count (5)
      0x81, 0x02, //     Input (Data,Var,Abs)
      0x75, 0x03, //     Report Size (3)
      0x95, 0x01, //     Report Count (1)
      0x81, 0x01, //     Input (Cnst)
  };
  // clang-format on

  if (report_descriptor.size() < descriptor_prefix.size() ||
      !std::equal(std::begin(descriptor_prefix),
                  std::end(descriptor_prefix),
                  std::begin(report_descriptor))) {
    return std::nullopt;
  }

  return configuration{
      .report_id = 1,
      .buttons_byte_index = 1, // Leading report id, then the button byte.
      .button_mask = 0xe0,     // Buttons 6, 7 and 8.
  };
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
        last_buttons_(0) {
  }

  [[nodiscard]] std::vector<button_change> update(uint8_t report_id,
                                                  std::span<const uint8_t> report) {
    std::vector<button_change> result;

    if (report_id != configuration_.report_id ||
        configuration_.buttons_byte_index >= report.size()) {
      return result;
    }

    auto buttons = static_cast<uint8_t>(report[configuration_.buttons_byte_index] &
                                        configuration_.button_mask);
    auto changed = static_cast<uint8_t>(buttons ^ last_buttons_);
    last_buttons_ = buttons;

    for (uint8_t bit = 0; bit < 8; ++bit) {
      auto mask = static_cast<uint8_t>(1u << bit);
      if (changed & mask) {
        result.push_back(button_change{
            .button = static_cast<uint8_t>(bit + 1),
            .pressed = (buttons & mask) != 0,
        });
      }
    }

    return result;
  }

  void reset() {
    last_buttons_ = 0;
  }

private:
  configuration configuration_;
  uint8_t last_buttons_;
};
} // namespace krbn::undeclared_buttons
