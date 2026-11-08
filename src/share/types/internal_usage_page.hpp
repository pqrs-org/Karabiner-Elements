#pragma once

#include <pqrs/hid.hpp>

namespace krbn::internal_usage_page {
// Negative values are reserved for logical events generated inside
// Karabiner-Elements and cannot collide with physical HID usage pages.
constexpr pqrs::hid::usage_page::value_t logitech_hidpp_button(-1);
} // namespace krbn::internal_usage_page
