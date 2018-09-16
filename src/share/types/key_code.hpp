#pragma once

#include "stream_utility.hpp"
#include <cstdint>

namespace krbn {
enum class key_code : uint32_t {
  // 0x00 - 0xff are keys in keyboard_or_keypad usage page.

  a = kHIDUsage_KeyboardA,
  b = kHIDUsage_KeyboardB,
  c = kHIDUsage_KeyboardC,
  d = kHIDUsage_KeyboardD,
  e = kHIDUsage_KeyboardE,
  f = kHIDUsage_KeyboardF,
  g = kHIDUsage_KeyboardG,
  h = kHIDUsage_KeyboardH,
  i = kHIDUsage_KeyboardI,
  j = kHIDUsage_KeyboardJ,
  k = kHIDUsage_KeyboardK,
  l = kHIDUsage_KeyboardL,
  m = kHIDUsage_KeyboardM,
  n = kHIDUsage_KeyboardN,
  o = kHIDUsage_KeyboardO,
  p = kHIDUsage_KeyboardP,
  q = kHIDUsage_KeyboardQ,
  r = kHIDUsage_KeyboardR,
  s = kHIDUsage_KeyboardS,
  t = kHIDUsage_KeyboardT,
  u = kHIDUsage_KeyboardU,
  v = kHIDUsage_KeyboardV,
  w = kHIDUsage_KeyboardW,
  x = kHIDUsage_KeyboardX,
  y = kHIDUsage_KeyboardY,
  z = kHIDUsage_KeyboardZ,

  return_or_enter = kHIDUsage_KeyboardReturnOrEnter,
  escape = kHIDUsage_KeyboardEscape,
  delete_or_backspace = kHIDUsage_KeyboardDeleteOrBackspace,
  tab = kHIDUsage_KeyboardTab,
  spacebar = kHIDUsage_KeyboardSpacebar,

  caps_lock = kHIDUsage_KeyboardCapsLock,

  f1 = kHIDUsage_KeyboardF1,
  f2 = kHIDUsage_KeyboardF2,
  f3 = kHIDUsage_KeyboardF3,
  f4 = kHIDUsage_KeyboardF4,
  f5 = kHIDUsage_KeyboardF5,
  f6 = kHIDUsage_KeyboardF6,
  f7 = kHIDUsage_KeyboardF7,
  f8 = kHIDUsage_KeyboardF8,
  f9 = kHIDUsage_KeyboardF9,
  f10 = kHIDUsage_KeyboardF10,
  f11 = kHIDUsage_KeyboardF11,
  f12 = kHIDUsage_KeyboardF12,
  f13 = kHIDUsage_KeyboardF13,
  f14 = kHIDUsage_KeyboardF14,
  f15 = kHIDUsage_KeyboardF15,
  f16 = kHIDUsage_KeyboardF16,
  f17 = kHIDUsage_KeyboardF17,
  f18 = kHIDUsage_KeyboardF18,
  f19 = kHIDUsage_KeyboardF19,
  f20 = kHIDUsage_KeyboardF20,
  f21 = kHIDUsage_KeyboardF21,
  f22 = kHIDUsage_KeyboardF22,
  f23 = kHIDUsage_KeyboardF23,
  f24 = kHIDUsage_KeyboardF24,

  right_arrow = kHIDUsage_KeyboardRightArrow,
  left_arrow = kHIDUsage_KeyboardLeftArrow,
  down_arrow = kHIDUsage_KeyboardDownArrow,
  up_arrow = kHIDUsage_KeyboardUpArrow,

  keypad_slash = kHIDUsage_KeypadSlash,
  keypad_asterisk = kHIDUsage_KeypadAsterisk,
  keypad_hyphen = kHIDUsage_KeypadHyphen,
  keypad_plus = kHIDUsage_KeypadPlus,
  keypad_enter = kHIDUsage_KeypadEnter,
  keypad_1 = kHIDUsage_Keypad1,
  keypad_2 = kHIDUsage_Keypad2,
  keypad_3 = kHIDUsage_Keypad3,
  keypad_4 = kHIDUsage_Keypad4,
  keypad_5 = kHIDUsage_Keypad5,
  keypad_6 = kHIDUsage_Keypad6,
  keypad_7 = kHIDUsage_Keypad7,
  keypad_8 = kHIDUsage_Keypad8,
  keypad_9 = kHIDUsage_Keypad9,
  keypad_0 = kHIDUsage_Keypad0,
  keypad_period = kHIDUsage_KeypadPeriod,
  keypad_equal_sign = kHIDUsage_KeypadEqualSign,
  keypad_comma = kHIDUsage_KeypadComma,

  home = kHIDUsage_KeyboardHome,
  page_up = kHIDUsage_KeyboardPageUp,
  delete_forward = kHIDUsage_KeyboardDeleteForward,
  end = kHIDUsage_KeyboardEnd,
  page_down = kHIDUsage_KeyboardPageDown,

  mute = kHIDUsage_KeyboardMute,
  volume_decrement = kHIDUsage_KeyboardVolumeDown,
  volume_increment = kHIDUsage_KeyboardVolumeUp,

  left_control = kHIDUsage_KeyboardLeftControl,
  left_shift = kHIDUsage_KeyboardLeftShift,
  left_option = kHIDUsage_KeyboardLeftAlt,
  left_command = kHIDUsage_KeyboardLeftGUI,
  right_control = kHIDUsage_KeyboardRightControl,
  right_shift = kHIDUsage_KeyboardRightShift,
  right_option = kHIDUsage_KeyboardRightAlt,
  right_command = kHIDUsage_KeyboardRightGUI,

  // usage in keyboard_or_keypad usage page is reserved until 0xffff.

  // 0x10000 - are karabiner own virtual key codes or keys not in keyboard_or_keypad usage page.
  extra_ = 0x10000,
  // A pseudo key that does not send any event.
  vk_none,

  // Keys that are not in generic keyboard_or_keypad usage_page.
  fn,
  display_brightness_decrement,
  display_brightness_increment,
  dashboard,
  launchpad,
  mission_control,
  illumination_decrement,
  illumination_increment,
  rewind,
  play_or_pause,
  fastforward,
  eject,
  apple_display_brightness_decrement,
  apple_display_brightness_increment,
  apple_top_case_display_brightness_decrement,
  apple_top_case_display_brightness_increment,
};

inline std::ostream& operator<<(std::ostream& stream, const key_code& value) {
  return stream_utility::output_enum(stream, value);
}

template <template <class T, class A> class container>
inline std::ostream& operator<<(std::ostream& stream, const container<key_code, std::allocator<key_code>>& values) {
  return stream_utility::output_enums(stream, values);
}

template <template <class T, class H, class K, class A> class container>
inline std::ostream& operator<<(std::ostream& stream, const container<key_code, std::hash<key_code>, std::equal_to<key_code>, std::allocator<key_code>>& values) {
  return stream_utility::output_enums(stream, values);
}
} // namespace krbn
