#pragma once

#include "boost_defs.hpp"

#include "Karabiner-VirtualHIDDevice/dist/include/karabiner_virtual_hid_device_methods.hpp"
#include "apple_hid_usage_tables.hpp"
#include "constants.hpp"
#include "stream_utility.hpp"
#include "system_preferences.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <IOKit/hidsystem/ev_keymap.h>
#include <boost/optional.hpp>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace krbn {
enum class operation_type : uint8_t {
  none,
  // console_user_server -> grabber
  connect,
  system_preferences_values_updated,
};

enum class device_id : uint32_t {
  zero = 0,
};

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
  csmr_acpan = kHIDUsage_Csmr_ACPan,

  apple_vendor_keyboard_function = kHIDUsage_AppleVendorKeyboard_Function,
  av_top_case_keyboard_fn = kHIDUsage_AV_TopCase_KeyboardFn,
};

enum class event_type : uint32_t {
  key_down,
  key_up,
};

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

  // 0x1000 - are karabiner own virtual key codes or keys not in keyboard_or_keypad usage page.
  extra_ = 0x1000,
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

enum class pointing_button : uint32_t {
  zero,

  button1,
  button2,
  button3,
  button4,
  button5,
  button6,
  button7,
  button8,

  button9,
  button10,
  button11,
  button12,
  button13,
  button14,
  button15,
  button16,

  button17,
  button18,
  button19,
  button20,
  button21,
  button22,
  button23,
  button24,

  button25,
  button26,
  button27,
  button28,
  button29,
  button30,
  button31,
  button32,

  end_,
};

enum class pointing_event : uint32_t {
  button,
  x,
  y,
  vertical_wheel,
  horizontal_wheel,
};

enum class modifier_flag : uint32_t {
  zero,
  caps_lock,
  left_control,
  left_shift,
  left_option,
  left_command,
  right_control,
  right_shift,
  right_option,
  right_command,
  fn,
  end_,
};

enum class led_state : uint32_t {
  on,
  off,
};

enum class vendor_id : uint32_t {
  zero = 0,
};

enum class product_id : uint32_t {
  zero = 0,
};

enum class location_id : uint32_t {
};

class types final {
public:
  static device_id get_new_device_id(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static auto id = device_id::zero;

    id = device_id(static_cast<uint32_t>(id) + 1);
    return id;
  }

  static modifier_flag get_modifier_flag(key_code key_code) {
    // get_modifier_flag(key_code::caps_lock) == modifier_flag::zero.

    switch (key_code) {
      case key_code::left_control:
        return modifier_flag::left_control;
      case key_code::left_shift:
        return modifier_flag::left_shift;
      case key_code::left_option:
        return modifier_flag::left_option;
      case key_code::left_command:
        return modifier_flag::left_command;
      case key_code::right_control:
        return modifier_flag::right_control;
      case key_code::right_shift:
        return modifier_flag::right_shift;
      case key_code::right_option:
        return modifier_flag::right_option;
      case key_code::right_command:
        return modifier_flag::right_command;
      case key_code::fn:
        return modifier_flag::fn;
      default:
        return modifier_flag::zero;
    }
  }

  static boost::optional<key_code> get_key_code(modifier_flag modifier_flag) {
    switch (modifier_flag) {
      case modifier_flag::zero:
        return boost::none;

      case modifier_flag::caps_lock:
        return key_code::caps_lock;

      case modifier_flag::left_control:
        return key_code::left_control;

      case modifier_flag::left_shift:
        return key_code::left_shift;

      case modifier_flag::left_option:
        return key_code::left_option;

      case modifier_flag::left_command:
        return key_code::left_command;

      case modifier_flag::right_control:
        return key_code::right_control;

      case modifier_flag::right_shift:
        return key_code::right_shift;

      case modifier_flag::right_option:
        return key_code::right_option;

      case modifier_flag::right_command:
        return key_code::right_command;

      case modifier_flag::fn:
        return key_code::fn;

      case modifier_flag::end_:
        return boost::none;
    }
  }

  // string -> hid usage map
  static const std::unordered_map<std::string, key_code>& get_key_code_map(void) {
    static std::unordered_map<std::string, key_code> map({
        // From IOHIDUsageTables.h
        {"a", key_code(kHIDUsage_KeyboardA)},
        {"b", key_code(kHIDUsage_KeyboardB)},
        {"c", key_code(kHIDUsage_KeyboardC)},
        {"d", key_code(kHIDUsage_KeyboardD)},
        {"e", key_code(kHIDUsage_KeyboardE)},
        {"f", key_code(kHIDUsage_KeyboardF)},
        {"g", key_code(kHIDUsage_KeyboardG)},
        {"h", key_code(kHIDUsage_KeyboardH)},
        {"i", key_code(kHIDUsage_KeyboardI)},
        {"j", key_code(kHIDUsage_KeyboardJ)},
        {"k", key_code(kHIDUsage_KeyboardK)},
        {"l", key_code(kHIDUsage_KeyboardL)},
        {"m", key_code(kHIDUsage_KeyboardM)},
        {"n", key_code(kHIDUsage_KeyboardN)},
        {"o", key_code(kHIDUsage_KeyboardO)},
        {"p", key_code(kHIDUsage_KeyboardP)},
        {"q", key_code(kHIDUsage_KeyboardQ)},
        {"r", key_code(kHIDUsage_KeyboardR)},
        {"s", key_code(kHIDUsage_KeyboardS)},
        {"t", key_code(kHIDUsage_KeyboardT)},
        {"u", key_code(kHIDUsage_KeyboardU)},
        {"v", key_code(kHIDUsage_KeyboardV)},
        {"w", key_code(kHIDUsage_KeyboardW)},
        {"x", key_code(kHIDUsage_KeyboardX)},
        {"y", key_code(kHIDUsage_KeyboardY)},
        {"z", key_code(kHIDUsage_KeyboardZ)},
        {"1", key_code(kHIDUsage_Keyboard1)},
        {"2", key_code(kHIDUsage_Keyboard2)},
        {"3", key_code(kHIDUsage_Keyboard3)},
        {"4", key_code(kHIDUsage_Keyboard4)},
        {"5", key_code(kHIDUsage_Keyboard5)},
        {"6", key_code(kHIDUsage_Keyboard6)},
        {"7", key_code(kHIDUsage_Keyboard7)},
        {"8", key_code(kHIDUsage_Keyboard8)},
        {"9", key_code(kHIDUsage_Keyboard9)},
        {"0", key_code(kHIDUsage_Keyboard0)},
        {"return_or_enter", key_code(kHIDUsage_KeyboardReturnOrEnter)},
        {"escape", key_code(kHIDUsage_KeyboardEscape)},
        {"delete_or_backspace", key_code(kHIDUsage_KeyboardDeleteOrBackspace)},
        {"tab", key_code(kHIDUsage_KeyboardTab)},
        {"spacebar", key_code(kHIDUsage_KeyboardSpacebar)},
        {"hyphen", key_code(kHIDUsage_KeyboardHyphen)},
        {"equal_sign", key_code(kHIDUsage_KeyboardEqualSign)},
        {"open_bracket", key_code(kHIDUsage_KeyboardOpenBracket)},
        {"close_bracket", key_code(kHIDUsage_KeyboardCloseBracket)},
        {"backslash", key_code(kHIDUsage_KeyboardBackslash)},
        {"non_us_pound", key_code(kHIDUsage_KeyboardNonUSPound)},
        {"semicolon", key_code(kHIDUsage_KeyboardSemicolon)},
        {"quote", key_code(kHIDUsage_KeyboardQuote)},
        {"grave_accent_and_tilde", key_code(kHIDUsage_KeyboardGraveAccentAndTilde)},
        {"comma", key_code(kHIDUsage_KeyboardComma)},
        {"period", key_code(kHIDUsage_KeyboardPeriod)},
        {"slash", key_code(kHIDUsage_KeyboardSlash)},
        {"caps_lock", key_code(kHIDUsage_KeyboardCapsLock)},
        {"f1", key_code(kHIDUsage_KeyboardF1)},
        {"f2", key_code(kHIDUsage_KeyboardF2)},
        {"f3", key_code(kHIDUsage_KeyboardF3)},
        {"f4", key_code(kHIDUsage_KeyboardF4)},
        {"f5", key_code(kHIDUsage_KeyboardF5)},
        {"f6", key_code(kHIDUsage_KeyboardF6)},
        {"f7", key_code(kHIDUsage_KeyboardF7)},
        {"f8", key_code(kHIDUsage_KeyboardF8)},
        {"f9", key_code(kHIDUsage_KeyboardF9)},
        {"f10", key_code(kHIDUsage_KeyboardF10)},
        {"f11", key_code(kHIDUsage_KeyboardF11)},
        {"f12", key_code(kHIDUsage_KeyboardF12)},
        {"print_screen", key_code(kHIDUsage_KeyboardPrintScreen)},
        {"scroll_lock", key_code(kHIDUsage_KeyboardScrollLock)},
        {"pause", key_code(kHIDUsage_KeyboardPause)},
        {"insert", key_code(kHIDUsage_KeyboardInsert)},
        {"home", key_code(kHIDUsage_KeyboardHome)},
        {"page_up", key_code(kHIDUsage_KeyboardPageUp)},
        {"delete_forward", key_code(kHIDUsage_KeyboardDeleteForward)},
        {"end", key_code(kHIDUsage_KeyboardEnd)},
        {"page_down", key_code(kHIDUsage_KeyboardPageDown)},
        {"right_arrow", key_code(kHIDUsage_KeyboardRightArrow)},
        {"left_arrow", key_code(kHIDUsage_KeyboardLeftArrow)},
        {"down_arrow", key_code(kHIDUsage_KeyboardDownArrow)},
        {"up_arrow", key_code(kHIDUsage_KeyboardUpArrow)},
        {"keypad_num_lock", key_code(kHIDUsage_KeypadNumLock)},
        {"keypad_slash", key_code(kHIDUsage_KeypadSlash)},
        {"keypad_asterisk", key_code(kHIDUsage_KeypadAsterisk)},
        {"keypad_hyphen", key_code(kHIDUsage_KeypadHyphen)},
        {"keypad_plus", key_code(kHIDUsage_KeypadPlus)},
        {"keypad_enter", key_code(kHIDUsage_KeypadEnter)},
        {"keypad_1", key_code(kHIDUsage_Keypad1)},
        {"keypad_2", key_code(kHIDUsage_Keypad2)},
        {"keypad_3", key_code(kHIDUsage_Keypad3)},
        {"keypad_4", key_code(kHIDUsage_Keypad4)},
        {"keypad_5", key_code(kHIDUsage_Keypad5)},
        {"keypad_6", key_code(kHIDUsage_Keypad6)},
        {"keypad_7", key_code(kHIDUsage_Keypad7)},
        {"keypad_8", key_code(kHIDUsage_Keypad8)},
        {"keypad_9", key_code(kHIDUsage_Keypad9)},
        {"keypad_0", key_code(kHIDUsage_Keypad0)},
        {"keypad_period", key_code(kHIDUsage_KeypadPeriod)},
        {"non_us_backslash", key_code(kHIDUsage_KeyboardNonUSBackslash)},
        {"application", key_code(kHIDUsage_KeyboardApplication)},
        {"power", key_code(kHIDUsage_KeyboardPower)},
        {"keypad_equal_sign", key_code(kHIDUsage_KeypadEqualSign)},
        {"f13", key_code(kHIDUsage_KeyboardF13)},
        {"f14", key_code(kHIDUsage_KeyboardF14)},
        {"f15", key_code(kHIDUsage_KeyboardF15)},
        {"f16", key_code(kHIDUsage_KeyboardF16)},
        {"f17", key_code(kHIDUsage_KeyboardF17)},
        {"f18", key_code(kHIDUsage_KeyboardF18)},
        {"f19", key_code(kHIDUsage_KeyboardF19)},
        {"f20", key_code(kHIDUsage_KeyboardF20)},
        {"f21", key_code(kHIDUsage_KeyboardF21)},
        {"f22", key_code(kHIDUsage_KeyboardF22)},
        {"f23", key_code(kHIDUsage_KeyboardF23)},
        {"f24", key_code(kHIDUsage_KeyboardF24)},
        {"execute", key_code(kHIDUsage_KeyboardExecute)},
        {"help", key_code(kHIDUsage_KeyboardHelp)},
        {"menu", key_code(kHIDUsage_KeyboardMenu)},
        {"select", key_code(kHIDUsage_KeyboardSelect)},
        {"stop", key_code(kHIDUsage_KeyboardStop)},
        {"again", key_code(kHIDUsage_KeyboardAgain)},
        {"undo", key_code(kHIDUsage_KeyboardUndo)},
        {"cut", key_code(kHIDUsage_KeyboardCut)},
        {"copy", key_code(kHIDUsage_KeyboardCopy)},
        {"paste", key_code(kHIDUsage_KeyboardPaste)},
        {"find", key_code(kHIDUsage_KeyboardFind)},
        {"mute", key_code(kHIDUsage_KeyboardMute)},
        {"volume_decrement", key_code(kHIDUsage_KeyboardVolumeDown)},
        {"volume_increment", key_code(kHIDUsage_KeyboardVolumeUp)},
        {"locking_caps_lock", key_code(kHIDUsage_KeyboardLockingCapsLock)},
        {"locking_num_lock", key_code(kHIDUsage_KeyboardLockingNumLock)},
        {"locking_scroll_lock", key_code(kHIDUsage_KeyboardLockingScrollLock)},
        {"keypad_comma", key_code(kHIDUsage_KeypadComma)},
        {"keypad_equal_sign_as400", key_code(kHIDUsage_KeypadEqualSignAS400)},
        {"international1", key_code(kHIDUsage_KeyboardInternational1)},
        {"international2", key_code(kHIDUsage_KeyboardInternational2)},
        {"international3", key_code(kHIDUsage_KeyboardInternational3)},
        {"international4", key_code(kHIDUsage_KeyboardInternational4)},
        {"international5", key_code(kHIDUsage_KeyboardInternational5)},
        {"international6", key_code(kHIDUsage_KeyboardInternational6)},
        {"international7", key_code(kHIDUsage_KeyboardInternational7)},
        {"international8", key_code(kHIDUsage_KeyboardInternational8)},
        {"international9", key_code(kHIDUsage_KeyboardInternational9)},
        {"lang1", key_code(kHIDUsage_KeyboardLANG1)},
        {"lang2", key_code(kHIDUsage_KeyboardLANG2)},
        {"lang3", key_code(kHIDUsage_KeyboardLANG3)},
        {"lang4", key_code(kHIDUsage_KeyboardLANG4)},
        {"lang5", key_code(kHIDUsage_KeyboardLANG5)},
        {"lang6", key_code(kHIDUsage_KeyboardLANG6)},
        {"lang7", key_code(kHIDUsage_KeyboardLANG7)},
        {"lang8", key_code(kHIDUsage_KeyboardLANG8)},
        {"lang9", key_code(kHIDUsage_KeyboardLANG9)},
        {"alternate_erase", key_code(kHIDUsage_KeyboardAlternateErase)},
        {"sys_req_or_attention", key_code(kHIDUsage_KeyboardSysReqOrAttention)},
        {"cancel", key_code(kHIDUsage_KeyboardCancel)},
        {"clear", key_code(kHIDUsage_KeyboardClear)},
        {"prior", key_code(kHIDUsage_KeyboardPrior)},
        {"return", key_code(kHIDUsage_KeyboardReturn)},
        {"separator", key_code(kHIDUsage_KeyboardSeparator)},
        {"out", key_code(kHIDUsage_KeyboardOut)},
        {"oper", key_code(kHIDUsage_KeyboardOper)},
        {"clear_or_again", key_code(kHIDUsage_KeyboardClearOrAgain)},
        {"cr_sel_or_props", key_code(kHIDUsage_KeyboardCrSelOrProps)},
        {"ex_sel", key_code(kHIDUsage_KeyboardExSel)},
        {"left_control", key_code(kHIDUsage_KeyboardLeftControl)},
        {"left_shift", key_code(kHIDUsage_KeyboardLeftShift)},
        {"left_alt", key_code(kHIDUsage_KeyboardLeftAlt)},
        {"left_gui", key_code(kHIDUsage_KeyboardLeftGUI)},
        {"right_control", key_code(kHIDUsage_KeyboardRightControl)},
        {"right_shift", key_code(kHIDUsage_KeyboardRightShift)},
        {"right_alt", key_code(kHIDUsage_KeyboardRightAlt)},
        {"right_gui", key_code(kHIDUsage_KeyboardRightGUI)},

        // Extra
        {"vk_none", key_code::vk_none},

        {"fn", key_code::fn},
        {"display_brightness_decrement", key_code::display_brightness_decrement},
        {"display_brightness_increment", key_code::display_brightness_increment},
        {"mission_control", key_code::mission_control},
        {"launchpad", key_code::launchpad},
        {"dashboard", key_code::dashboard},
        {"illumination_decrement", key_code::illumination_decrement},
        {"illumination_increment", key_code::illumination_increment},
        {"rewind", key_code::rewind},
        {"play_or_pause", key_code::play_or_pause},
        {"fastforward", key_code::fastforward},
        {"eject", key_code::eject},
        {"apple_display_brightness_decrement", key_code::apple_display_brightness_decrement},
        {"apple_display_brightness_increment", key_code::apple_display_brightness_increment},
        {"apple_top_case_display_brightness_decrement", key_code::apple_top_case_display_brightness_decrement},
        {"apple_top_case_display_brightness_increment", key_code::apple_top_case_display_brightness_increment},

        // Aliases
        {"left_option", key_code(kHIDUsage_KeyboardLeftAlt)},
        {"left_command", key_code(kHIDUsage_KeyboardLeftGUI)},
        {"right_option", key_code(kHIDUsage_KeyboardRightAlt)},
        {"right_command", key_code(kHIDUsage_KeyboardRightGUI)},
        {"japanese_eisuu", key_code(kHIDUsage_KeyboardLANG2)},
        {"japanese_kana", key_code(kHIDUsage_KeyboardLANG1)},
        {"japanese_pc_nfer", key_code(kHIDUsage_KeyboardInternational5)},
        {"japanese_pc_xfer", key_code(kHIDUsage_KeyboardInternational4)},
        {"japanese_pc_katakana", key_code(kHIDUsage_KeyboardInternational2)},
        {"vk_consumer_brightness_down", key_code::display_brightness_decrement},
        {"vk_consumer_brightness_up", key_code::display_brightness_increment},
        {"vk_mission_control", key_code::mission_control},
        {"vk_launchpad", key_code::launchpad},
        {"vk_dashboard", key_code::dashboard},
        {"vk_consumer_illumination_down", key_code::illumination_decrement},
        {"vk_consumer_illumination_up", key_code::illumination_increment},
        {"vk_consumer_previous", key_code::rewind},
        {"vk_consumer_play", key_code::play_or_pause},
        {"vk_consumer_next", key_code::fastforward},
        {"volume_down", key_code(kHIDUsage_KeyboardVolumeDown)},
        {"volume_up", key_code(kHIDUsage_KeyboardVolumeUp)},
    });
    return map;
  }

  static boost::optional<key_code> get_key_code(const std::string& name) {
    auto& map = get_key_code_map();
    auto it = map.find(name);
    if (it == map.end()) {
      return boost::none;
    }
    return it->second;
  }

  static boost::optional<key_code> get_key_code(hid_usage_page usage_page, hid_usage usage) {
    auto u = static_cast<uint32_t>(usage);

    switch (usage_page) {
      case hid_usage_page::keyboard_or_keypad:
        if (kHIDUsage_KeyboardErrorUndefined < u && u < kHIDUsage_Keyboard_Reserved) {
          return key_code(u);
        }
        break;

      case hid_usage_page::apple_vendor_top_case:
        if (usage == hid_usage::av_top_case_keyboard_fn) {
          return key_code::fn;
        }
        break;

      case hid_usage_page::apple_vendor_keyboard:
        if (usage == hid_usage::apple_vendor_keyboard_function) {
          return key_code::fn;
        }
        break;

      default:
        break;
    }

    return boost::none;
  }

  static boost::optional<pqrs::karabiner_virtual_hid_device::usage_page> get_usage_page(key_code key_code) {
    switch (key_code) {
      case key_code::fn:
      case key_code::illumination_decrement:
      case key_code::illumination_increment:
      case key_code::apple_top_case_display_brightness_decrement:
      case key_code::apple_top_case_display_brightness_increment:
        return pqrs::karabiner_virtual_hid_device::usage_page::apple_vendor_top_case;

      case key_code::dashboard:
      case key_code::launchpad:
      case key_code::mission_control:
      case key_code::apple_display_brightness_decrement:
      case key_code::apple_display_brightness_increment:
        return pqrs::karabiner_virtual_hid_device::usage_page::apple_vendor_keyboard;

      case key_code::mute:
      case key_code::volume_decrement:
      case key_code::volume_increment:
      case key_code::display_brightness_decrement:
      case key_code::display_brightness_increment:
      case key_code::rewind:
      case key_code::play_or_pause:
      case key_code::fastforward:
      case key_code::eject:
        return pqrs::karabiner_virtual_hid_device::usage_page::consumer;

      case key_code::vk_none:
        return boost::none;

      default:
        return pqrs::karabiner_virtual_hid_device::usage_page::keyboard_or_keypad;
    }
  }

  static boost::optional<pqrs::karabiner_virtual_hid_device::usage> get_usage(key_code key_code) {
    switch (key_code) {
      case key_code::fn:
        return pqrs::karabiner_virtual_hid_device::usage::av_top_case_keyboard_fn;

      case key_code::illumination_decrement:
        return pqrs::karabiner_virtual_hid_device::usage::av_top_case_illumination_down;

      case key_code::illumination_increment:
        return pqrs::karabiner_virtual_hid_device::usage::av_top_case_illumination_up;

      case key_code::apple_top_case_display_brightness_decrement:
        return pqrs::karabiner_virtual_hid_device::usage::av_top_case_brightness_down;

      case key_code::apple_top_case_display_brightness_increment:
        return pqrs::karabiner_virtual_hid_device::usage::av_top_case_brightness_up;

      case key_code::dashboard:
        return pqrs::karabiner_virtual_hid_device::usage::apple_vendor_keyboard_dashboard;

      case key_code::launchpad:
        return pqrs::karabiner_virtual_hid_device::usage::apple_vendor_keyboard_launchpad;

      case key_code::mission_control:
        return pqrs::karabiner_virtual_hid_device::usage::apple_vendor_keyboard_expose_all;

      case key_code::apple_display_brightness_decrement:
        return pqrs::karabiner_virtual_hid_device::usage::apple_vendor_keyboard_brightness_down;

      case key_code::apple_display_brightness_increment:
        return pqrs::karabiner_virtual_hid_device::usage::apple_vendor_keyboard_brightness_up;

      case key_code::mute:
        return pqrs::karabiner_virtual_hid_device::usage::csmr_mute;

      case key_code::volume_decrement:
        return pqrs::karabiner_virtual_hid_device::usage::csmr_volume_decrement;

      case key_code::volume_increment:
        return pqrs::karabiner_virtual_hid_device::usage::csmr_volume_increment;

      case key_code::display_brightness_decrement:
        return pqrs::karabiner_virtual_hid_device::usage::csmr_display_brightness_decrement;

      case key_code::display_brightness_increment:
        return pqrs::karabiner_virtual_hid_device::usage::csmr_display_brightness_increment;

      case key_code::rewind:
        return pqrs::karabiner_virtual_hid_device::usage::csmr_rewind;

      case key_code::play_or_pause:
        return pqrs::karabiner_virtual_hid_device::usage::csmr_play_or_pause;

      case key_code::fastforward:
        return pqrs::karabiner_virtual_hid_device::usage::csmr_fastforward;

      case key_code::eject:
        return pqrs::karabiner_virtual_hid_device::usage::csmr_eject;

      case key_code::vk_none:
        return boost::none;

      default:
        return pqrs::karabiner_virtual_hid_device::usage(key_code);
    }
  }

  static const std::unordered_map<std::string, pointing_button>& get_pointing_button_map(void) {
    static std::unordered_map<std::string, pointing_button> map({
        // From IOHIDUsageTables.h

        {"button1", pointing_button::button1},
        {"button2", pointing_button::button2},
        {"button3", pointing_button::button3},
        {"button4", pointing_button::button4},
        {"button5", pointing_button::button5},
        {"button6", pointing_button::button6},
        {"button7", pointing_button::button7},
        {"button8", pointing_button::button8},

        {"button9", pointing_button::button9},
        {"button10", pointing_button::button10},
        {"button11", pointing_button::button11},
        {"button12", pointing_button::button12},
        {"button13", pointing_button::button13},
        {"button14", pointing_button::button14},
        {"button15", pointing_button::button15},
        {"button16", pointing_button::button16},

        {"button17", pointing_button::button17},
        {"button18", pointing_button::button18},
        {"button19", pointing_button::button19},
        {"button20", pointing_button::button20},
        {"button21", pointing_button::button21},
        {"button22", pointing_button::button22},
        {"button23", pointing_button::button23},
        {"button24", pointing_button::button24},

        {"button25", pointing_button::button25},
        {"button26", pointing_button::button26},
        {"button27", pointing_button::button27},
        {"button28", pointing_button::button28},
        {"button29", pointing_button::button29},
        {"button30", pointing_button::button30},
        {"button31", pointing_button::button31},
        {"button32", pointing_button::button32},
    });
    return map;
  }

  static boost::optional<pointing_button> get_pointing_button(const std::string& name) {
    auto& map = get_pointing_button_map();
    auto it = map.find(name);
    if (it == map.end()) {
      return boost::none;
    }
    return it->second;
  }

  static boost::optional<pointing_button> get_pointing_button(hid_usage_page usage_page, hid_usage usage) {
    if (usage_page == hid_usage_page::button) {
      return pointing_button(usage);
    }
    return boost::none;
  }

  static const std::unordered_map<std::string, pqrs::karabiner_virtual_hid_device::properties::keyboard_type>& get_keyboard_type_map(void) {
    static std::unordered_map<std::string, pqrs::karabiner_virtual_hid_device::properties::keyboard_type> map({
        {"none", pqrs::karabiner_virtual_hid_device::properties::keyboard_type::none},
        {"ansi", pqrs::karabiner_virtual_hid_device::properties::keyboard_type::ansi},
        {"iso", pqrs::karabiner_virtual_hid_device::properties::keyboard_type::iso},
        {"jis", pqrs::karabiner_virtual_hid_device::properties::keyboard_type::jis},
    });

    return map;
  }

  static boost::optional<pqrs::karabiner_virtual_hid_device::properties::keyboard_type> get_keyboard_type(const std::string& name) {
    auto& map = get_keyboard_type_map();
    auto it = map.find(name);
    if (it == map.end()) {
      return boost::none;
    }
    return it->second;
  }
};

struct operation_type_connect_struct {
  operation_type_connect_struct(void) : operation_type(operation_type::connect) {
    strlcpy(user_core_configuration_file_path,
            constants::get_user_core_configuration_file_path().c_str(),
            sizeof(user_core_configuration_file_path));
  }

  const operation_type operation_type;
  pid_t pid;
  char user_core_configuration_file_path[_POSIX_PATH_MAX];
};

struct operation_type_system_preferences_values_updated_struct {
  operation_type_system_preferences_values_updated_struct(void) : operation_type(operation_type::system_preferences_values_updated) {}

  const operation_type operation_type;
  system_preferences::values values;
};

// stream output

#define KRBN_TYPES_STREAM_OUTPUT(TYPE)                                                                                                               \
  inline std::ostream& operator<<(std::ostream& stream, const TYPE& value) {                                                                         \
    return stream_utility::output_enum(stream, value);                                                                                               \
  }                                                                                                                                                  \
                                                                                                                                                     \
  template <template <class T, class A> class container>                                                                                             \
  inline std::ostream& operator<<(std::ostream& stream, const container<TYPE, std::allocator<TYPE>>& values) {                                       \
    return stream_utility::output_enums(stream, values);                                                                                             \
  }                                                                                                                                                  \
                                                                                                                                                     \
  template <template <class T, class H, class K, class A> class container>                                                                           \
  inline std::ostream& operator<<(std::ostream& stream, const container<TYPE, std::hash<TYPE>, std::equal_to<TYPE>, std::allocator<TYPE>>& values) { \
    return stream_utility::output_enums(stream, values);                                                                                             \
  }

KRBN_TYPES_STREAM_OUTPUT(device_id);
KRBN_TYPES_STREAM_OUTPUT(event_type);
KRBN_TYPES_STREAM_OUTPUT(key_code);
KRBN_TYPES_STREAM_OUTPUT(modifier_flag);
KRBN_TYPES_STREAM_OUTPUT(pointing_button);

#undef KRBN_TYPES_STREAM_OUTPUT

} // namespace krbn
