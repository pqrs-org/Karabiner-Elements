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
struct iokit_hid_usage_page : type_safe::strong_typedef<iokit_hid_usage_page, int32_t>,
                              type_safe::strong_typedef_op::equality_comparison<iokit_hid_usage_page>,
                              type_safe::strong_typedef_op::relational_comparison<iokit_hid_usage_page> {
  using strong_typedef::strong_typedef;
};

inline std::ostream& operator<<(std::ostream& stream, const iokit_hid_usage_page& value) {
  return stream << type_safe::get(value);
}

// from IOHIDUsageTables.h

constexpr iokit_hid_usage_page iokit_hid_usage_page_generic_desktop(kHIDPage_GenericDesktop);
constexpr iokit_hid_usage_page iokit_hid_usage_page_simulation(kHIDPage_Simulation);
constexpr iokit_hid_usage_page iokit_hid_usage_page_vr(kHIDPage_VR);
constexpr iokit_hid_usage_page iokit_hid_usage_page_sport(kHIDPage_Sport);
constexpr iokit_hid_usage_page iokit_hid_usage_page_game(kHIDPage_Game);
constexpr iokit_hid_usage_page iokit_hid_usage_page_generic_device_controls(kHIDPage_GenericDeviceControls);
constexpr iokit_hid_usage_page iokit_hid_usage_page_keyboard_or_keypad(kHIDPage_KeyboardOrKeypad);
constexpr iokit_hid_usage_page iokit_hid_usage_page_leds(kHIDPage_LEDs);
constexpr iokit_hid_usage_page iokit_hid_usage_page_button(kHIDPage_Button);
constexpr iokit_hid_usage_page iokit_hid_usage_page_ordinal(kHIDPage_Ordinal);
constexpr iokit_hid_usage_page iokit_hid_usage_page_telephony(kHIDPage_Telephony);
constexpr iokit_hid_usage_page iokit_hid_usage_page_consumer(kHIDPage_Consumer);
constexpr iokit_hid_usage_page iokit_hid_usage_page_digitizer(kHIDPage_Digitizer);
constexpr iokit_hid_usage_page iokit_hid_usage_page_pid(kHIDPage_PID);
constexpr iokit_hid_usage_page iokit_hid_usage_page_unicode(kHIDPage_Unicode);
constexpr iokit_hid_usage_page iokit_hid_usage_page_alphanumeric_display(kHIDPage_AlphanumericDisplay);
constexpr iokit_hid_usage_page iokit_hid_usage_page_sensor(kHIDPage_Sensor);
constexpr iokit_hid_usage_page iokit_hid_usage_page_monitor(kHIDPage_Monitor);
constexpr iokit_hid_usage_page iokit_hid_usage_page_monitorEnumerated(kHIDPage_MonitorEnumerated);
constexpr iokit_hid_usage_page iokit_hid_usage_page_monitorVirtual(kHIDPage_MonitorVirtual);
constexpr iokit_hid_usage_page iokit_hid_usage_page_monitorReserved(kHIDPage_MonitorReserved);
constexpr iokit_hid_usage_page iokit_hid_usage_page_power_device(kHIDPage_PowerDevice);
constexpr iokit_hid_usage_page iokit_hid_usage_page_battery_system(kHIDPage_BatterySystem);
constexpr iokit_hid_usage_page iokit_hid_usage_page_power_reserved(kHIDPage_PowerReserved);
constexpr iokit_hid_usage_page iokit_hid_usage_page_power_reserved2(kHIDPage_PowerReserved2);
constexpr iokit_hid_usage_page iokit_hid_usage_page_bar_code_scanner(kHIDPage_BarCodeScanner);
constexpr iokit_hid_usage_page iokit_hid_usage_page_weighing_device(kHIDPage_WeighingDevice);
constexpr iokit_hid_usage_page iokit_hid_usage_page_scale(kHIDPage_Scale);
constexpr iokit_hid_usage_page iokit_hid_usage_page_magnetic_stripe_reader(kHIDPage_MagneticStripeReader);
constexpr iokit_hid_usage_page iokit_hid_usage_page_camera_control(kHIDPage_CameraControl);
constexpr iokit_hid_usage_page iokit_hid_usage_page_arcade(kHIDPage_Arcade);

// from AppleHIDUsageTables.h

constexpr iokit_hid_usage_page iokit_hid_usage_page_apple_vendor(0xff00);
constexpr iokit_hid_usage_page iokit_hid_usage_page_apple_vendor_keyboard(0xff01);
constexpr iokit_hid_usage_page iokit_hid_usage_page_apple_vendor_mouse(0xff02);
constexpr iokit_hid_usage_page iokit_hid_usage_page_apple_vendor_accelerometer(0xff03);
constexpr iokit_hid_usage_page iokit_hid_usage_page_apple_vendor_ambient_light_sensor(0xff04);
constexpr iokit_hid_usage_page iokit_hid_usage_page_apple_vendor_temperature_sensor(0xff05);
constexpr iokit_hid_usage_page iokit_hid_usage_page_apple_vendor_headset(0xff07);
constexpr iokit_hid_usage_page iokit_hid_usage_page_apple_vendor_power_sensor(0xff08);
constexpr iokit_hid_usage_page iokit_hid_usage_page_apple_vendor_smart_cover(0xff09);
constexpr iokit_hid_usage_page iokit_hid_usage_page_apple_vendor_platinum(0xff0A);
constexpr iokit_hid_usage_page iokit_hid_usage_page_apple_vendor_lisa(0xff0B);
constexpr iokit_hid_usage_page iokit_hid_usage_page_apple_vendor_motion(0xff0C);
constexpr iokit_hid_usage_page iokit_hid_usage_page_apple_vendor_battery(0xff0D);
constexpr iokit_hid_usage_page iokit_hid_usage_page_apple_vendor_ir_remote(0xff0E);
constexpr iokit_hid_usage_page iokit_hid_usage_page_apple_vendor_debug(0xff0F);
constexpr iokit_hid_usage_page iokit_hid_usage_page_apple_vendor_ir_interface(0xff10);
constexpr iokit_hid_usage_page iokit_hid_usage_page_apple_vendor_filtered_event(0xff50);
constexpr iokit_hid_usage_page iokit_hid_usage_page_apple_vendor_multitouch(0xff60);
constexpr iokit_hid_usage_page iokit_hid_usage_page_apple_vendor_display(0xff92);
constexpr iokit_hid_usage_page iokit_hid_usage_page_apple_vendor_top_case(0x00ff);
} // namespace osx
} // namespace pqrs

namespace std {
template <>
struct hash<pqrs::osx::iokit_hid_usage_page> : type_safe::hashable<pqrs::osx::iokit_hid_usage_page> {
};
} // namespace std
