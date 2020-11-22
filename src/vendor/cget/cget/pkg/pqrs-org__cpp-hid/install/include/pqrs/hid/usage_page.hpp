#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <compare>
#include <functional>
#include <iostream>
#include <type_safe/strong_typedef.hpp>

namespace pqrs {
namespace hid {
namespace usage_page {
struct value_t : type_safe::strong_typedef<value_t, int32_t>,
                 type_safe::strong_typedef_op::equality_comparison<value_t>,
                 type_safe::strong_typedef_op::relational_comparison<value_t> {
  using strong_typedef::strong_typedef;

  constexpr auto operator<=>(const value_t& other) const {
    return type_safe::get(*this) <=> type_safe::get(other);
  }
};

inline std::ostream& operator<<(std::ostream& stream, const value_t& value) {
  return stream << type_safe::get(value);
}

//
// Values from HID Usage Tables Version 1.12.
// https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf
//

constexpr value_t undefined(0x00);
constexpr value_t generic_desktop(0x01);
constexpr value_t simulation(0x02);
constexpr value_t vr(0x03);
constexpr value_t sport(0x04);
constexpr value_t game(0x05);
constexpr value_t generic_device_controls(0x06);
constexpr value_t keyboard_or_keypad(0x07);
constexpr value_t leds(0x08);
constexpr value_t button(0x09);
constexpr value_t ordinal(0x0a);
constexpr value_t telephony(0x0b);
constexpr value_t consumer(0x0c);
constexpr value_t digitizer(0x0d);
// Reserved 0x0e
constexpr value_t pid(0x0f);
constexpr value_t unicode(0x10);
// Reserved 0x11 - 0x13
constexpr value_t alphanumeric_display(0x14);
// Reserved 0x15 - 0x3f

//
// Values from AppleHIDUsageTables.h
//

constexpr value_t apple_vendor(0xff00);
constexpr value_t apple_vendor_keyboard(0xff01);
constexpr value_t apple_vendor_mouse(0xff02);
constexpr value_t apple_vendor_accelerometer(0xff03);
constexpr value_t apple_vendor_ambient_light_sensor(0xff04);
constexpr value_t apple_vendor_temperature_sensor(0xff05);
constexpr value_t apple_vendor_headset(0xff07);
constexpr value_t apple_vendor_power_sensor(0xff08);
constexpr value_t apple_vendor_smart_cover(0xff09);
constexpr value_t apple_vendor_platinum(0xff0A);
constexpr value_t apple_vendor_lisa(0xff0B);
constexpr value_t apple_vendor_motion(0xff0C);
constexpr value_t apple_vendor_battery(0xff0D);
constexpr value_t apple_vendor_ir_remote(0xff0E);
constexpr value_t apple_vendor_debug(0xff0F);
constexpr value_t apple_vendor_ir_interface(0xff10);
constexpr value_t apple_vendor_filtered_event(0xff50);
constexpr value_t apple_vendor_multitouch(0xff60);
constexpr value_t apple_vendor_display(0xff92);
constexpr value_t apple_vendor_top_case(0x00ff);
} // namespace usage_page
} // namespace hid
} // namespace pqrs

namespace std {
template <>
struct hash<pqrs::hid::usage_page::value_t> : type_safe::hashable<pqrs::hid::usage_page::value_t> {
};
} // namespace std
