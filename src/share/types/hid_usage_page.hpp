#pragma once

#include "stream_utility.hpp"
#include <cstdint>

namespace krbn {
enum class hid_usage_page : uint32_t {
  zero = 0,
  generic_desktop = kHIDPage_GenericDesktop,
  keyboard_or_keypad = kHIDPage_KeyboardOrKeypad,
  leds = kHIDPage_LEDs,
  button = kHIDPage_Button,
  consumer = kHIDPage_Consumer,
  apple_vendor_keyboard = kHIDPage_AppleVendorKeyboard,
  apple_vendor_top_case = kHIDPage_AppleVendorTopCase,
};

inline std::ostream& operator<<(std::ostream& stream, const hid_usage_page& value) {
  return stream_utility::output_enum(stream, value);
}

template <template <class T, class A> class container>
inline std::ostream& operator<<(std::ostream& stream, const container<hid_usage_page, std::allocator<hid_usage_page>>& values) {
  return stream_utility::output_enums(stream, values);
}

template <template <class T, class H, class K, class A> class container>
inline std::ostream& operator<<(std::ostream& stream, const container<hid_usage_page, std::hash<hid_usage_page>, std::equal_to<hid_usage_page>, std::allocator<hid_usage_page>>& values) {
  return stream_utility::output_enums(stream, values);
}
} // namespace krbn
