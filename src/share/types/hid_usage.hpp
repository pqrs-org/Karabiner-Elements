#pragma once

#include "apple_hid_usage_tables.hpp"
#include "stream_utility.hpp"
#include <IOKit/hid/IOHIDUsageTables.h>
#include <cstdint>

namespace krbn {
enum class hid_usage : uint32_t {
  zero = 0,

  gd_pointer = kHIDUsage_GD_Pointer,
  gd_mouse = kHIDUsage_GD_Mouse,
  gd_keyboard = kHIDUsage_GD_Keyboard,
  gd_x = kHIDUsage_GD_X,
  gd_y = kHIDUsage_GD_Y,
  gd_z = kHIDUsage_GD_Z,
  gd_wheel = kHIDUsage_GD_Wheel,

  led_caps_lock = kHIDUsage_LED_CapsLock,

  csmr_consumercontrol = kHIDUsage_Csmr_ConsumerControl,
  csmr_power = kHIDUsage_Csmr_Power,
  csmr_display_brightness_increment = kHIDUsage_Csmr_DisplayBrightnessIncrement,
  csmr_display_brightness_decrement = kHIDUsage_Csmr_DisplayBrightnessDecrement,
  csmr_fastforward = kHIDUsage_Csmr_FastForward,
  csmr_rewind = kHIDUsage_Csmr_Rewind,
  csmr_scan_next_track = kHIDUsage_Csmr_ScanNextTrack,
  csmr_scan_previous_track = kHIDUsage_Csmr_ScanPreviousTrack,
  csmr_eject = kHIDUsage_Csmr_Eject,
  csmr_play_or_pause = kHIDUsage_Csmr_PlayOrPause,
  csmr_mute = kHIDUsage_Csmr_Mute,
  csmr_volume_increment = kHIDUsage_Csmr_VolumeIncrement,
  csmr_volume_decrement = kHIDUsage_Csmr_VolumeDecrement,
  csmr_acpan = kHIDUsage_Csmr_ACPan,

  apple_vendor_keyboard_spotlight = kHIDUsage_AppleVendorKeyboard_Spotlight,
  apple_vendor_keyboard_dashboard = kHIDUsage_AppleVendorKeyboard_Dashboard,
  apple_vendor_keyboard_function = kHIDUsage_AppleVendorKeyboard_Function,
  apple_vendor_keyboard_launchpad = kHIDUsage_AppleVendorKeyboard_Launchpad,
  apple_vendor_keyboard_expose_all = kHIDUsage_AppleVendorKeyboard_Expose_All,
  apple_vendor_keyboard_expose_desktop = kHIDUsage_AppleVendorKeyboard_Expose_Desktop,
  apple_vendor_keyboard_brightness_up = kHIDUsage_AppleVendorKeyboard_Brightness_Up,
  apple_vendor_keyboard_brightness_down = kHIDUsage_AppleVendorKeyboard_Brightness_Down,

  av_top_case_keyboard_fn = kHIDUsage_AV_TopCase_KeyboardFn,
  av_top_case_brightness_up = kHIDUsage_AV_TopCase_BrightnessUp,
  av_top_case_brightness_down = kHIDUsage_AV_TopCase_BrightnessDown,
  av_top_case_video_mirror = kHIDUsage_AV_TopCase_VideoMirror,
  av_top_case_illumination_toggle = kHIDUsage_AV_TopCase_IlluminationToggle,
  av_top_case_illumination_up = kHIDUsage_AV_TopCase_IlluminationUp,
  av_top_case_illumination_down = kHIDUsage_AV_TopCase_IlluminationDown,
};

inline std::ostream& operator<<(std::ostream& stream, const hid_usage& value) {
  return stream_utility::output_enum(stream, value);
}

template <template <class T, class A> class container>
inline std::ostream& operator<<(std::ostream& stream, const container<hid_usage, std::allocator<hid_usage>>& values) {
  return stream_utility::output_enums(stream, values);
}

template <template <class T, class H, class K, class A> class container>
inline std::ostream& operator<<(std::ostream& stream, const container<hid_usage, std::hash<hid_usage>, std::equal_to<hid_usage>, std::allocator<hid_usage>>& values) {
  return stream_utility::output_enums(stream, values);
}
} // namespace krbn
