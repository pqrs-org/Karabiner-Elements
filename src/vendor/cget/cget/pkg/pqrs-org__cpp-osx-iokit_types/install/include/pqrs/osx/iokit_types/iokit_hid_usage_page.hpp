#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <IOKit/hid/IOHIDUsageTables.h>
#include <functional>
#include <iostream>
#include <type_safe/strong_typedef.hpp>

namespace pqrs {
namespace osx {
namespace iokit_hid_usage_page {
struct value_t : type_safe::strong_typedef<value_t, int32_t>,
                 type_safe::strong_typedef_op::equality_comparison<value_t>,
                 type_safe::strong_typedef_op::relational_comparison<value_t> {
  using strong_typedef::strong_typedef;
};

inline std::ostream& operator<<(std::ostream& stream, const value_t& value) {
  return stream << type_safe::get(value);
}

//
// values from IOHIDUsageTables.h
//

constexpr value_t generic_desktop(kHIDPage_GenericDesktop);
constexpr value_t simulation(kHIDPage_Simulation);
constexpr value_t vr(kHIDPage_VR);
constexpr value_t sport(kHIDPage_Sport);
constexpr value_t game(kHIDPage_Game);
constexpr value_t generic_device_controls(kHIDPage_GenericDeviceControls);
constexpr value_t keyboard_or_keypad(kHIDPage_KeyboardOrKeypad);
constexpr value_t leds(kHIDPage_LEDs);
constexpr value_t button(kHIDPage_Button);
constexpr value_t ordinal(kHIDPage_Ordinal);
constexpr value_t telephony(kHIDPage_Telephony);
constexpr value_t consumer(kHIDPage_Consumer);
constexpr value_t digitizer(kHIDPage_Digitizer);
constexpr value_t pid(kHIDPage_PID);
constexpr value_t unicode(kHIDPage_Unicode);
constexpr value_t alphanumeric_display(kHIDPage_AlphanumericDisplay);
constexpr value_t sensor(kHIDPage_Sensor);
constexpr value_t monitor(kHIDPage_Monitor);
constexpr value_t monitorEnumerated(kHIDPage_MonitorEnumerated);
constexpr value_t monitorVirtual(kHIDPage_MonitorVirtual);
constexpr value_t monitorReserved(kHIDPage_MonitorReserved);
constexpr value_t power_device(kHIDPage_PowerDevice);
constexpr value_t battery_system(kHIDPage_BatterySystem);
constexpr value_t power_reserved(kHIDPage_PowerReserved);
constexpr value_t power_reserved2(kHIDPage_PowerReserved2);
constexpr value_t bar_code_scanner(kHIDPage_BarCodeScanner);
constexpr value_t weighing_device(kHIDPage_WeighingDevice);
constexpr value_t scale(kHIDPage_Scale);
constexpr value_t magnetic_stripe_reader(kHIDPage_MagneticStripeReader);
constexpr value_t camera_control(kHIDPage_CameraControl);
constexpr value_t arcade(kHIDPage_Arcade);

// from AppleHIDUsageTables.h

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
} // namespace iokit_hid_usage_page
} // namespace osx
} // namespace pqrs

namespace std {
template <>
struct hash<pqrs::osx::iokit_hid_usage_page::value_t> : type_safe::hashable<pqrs::osx::iokit_hid_usage_page::value_t> {
};
} // namespace std
