#pragma once

#include "boost_defs.hpp"

#include "Karabiner-VirtualHIDDevice/dist/include/karabiner_virtual_hid_device_methods.hpp"
#include "apple_hid_usage_tables.hpp"
#include "system_preferences.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <IOKit/hidsystem/ev_keymap.h>
#include <boost/optional.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace krbn {
enum class operation_type : uint8_t {
  none,
  // console_user_server -> grabber
  connect,
  system_preferences_values_updated,
  clear_simple_modifications,
  add_simple_modification,
  clear_fn_function_keys,
  add_fn_function_key,
  virtual_hid_keyboard_configuration_updated,
  clear_devices,
  add_device,
  complete_devices,
};

enum class connect_from : uint8_t {
  console_user_server,
};

enum class event_type : uint32_t {
  key_down,
  key_up,
};

enum class key_code : uint32_t {
  // 0x00 - 0xff are keys in keyboard_or_keypad usage page.
  return_or_enter = kHIDUsage_KeyboardReturnOrEnter,
  delete_or_backspace = kHIDUsage_KeyboardDeleteOrBackspace,
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
  none,
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
  prepared_modifier_flag_end_
};

enum class led_state : uint32_t {
  on,
  off,
};

enum class vendor_id : uint32_t {
};

enum class product_id : uint32_t {
};

enum class location_id : uint32_t {
};

enum class keyboard_type : uint32_t {
  none = 0,
  ansi = 40,
  iso = 41,
  jis = 42,
};

struct virtual_hid_keyboard_configuration_struct {
  virtual_hid_keyboard_configuration_struct(void) : keyboard_type(keyboard_type::ansi),
                                                    caps_lock_delay_milliseconds(0) {}

  bool operator==(const virtual_hid_keyboard_configuration_struct& other) const {
    return keyboard_type == other.keyboard_type &&
           caps_lock_delay_milliseconds == other.caps_lock_delay_milliseconds;
  }

  keyboard_type keyboard_type;
  uint32_t caps_lock_delay_milliseconds;
};

struct device_identifiers_struct {
  vendor_id vendor_id;
  product_id product_id;
  bool is_keyboard;
  bool is_pointing_device;
};

struct device_configuration_struct {
  bool ignore;
  bool disable_built_in_keyboard_if_exists;
};

class types final {
public:
  static modifier_flag get_modifier_flag(key_code key_code) {
    switch (static_cast<uint32_t>(key_code)) {
    case kHIDUsage_KeyboardLeftControl:
      return modifier_flag::left_control;
    case kHIDUsage_KeyboardLeftShift:
      return modifier_flag::left_shift;
    case kHIDUsage_KeyboardLeftAlt:
      return modifier_flag::left_option;
    case kHIDUsage_KeyboardLeftGUI:
      return modifier_flag::left_command;
    case kHIDUsage_KeyboardRightControl:
      return modifier_flag::right_control;
    case kHIDUsage_KeyboardRightShift:
      return modifier_flag::right_shift;
    case kHIDUsage_KeyboardRightAlt:
      return modifier_flag::right_option;
    case kHIDUsage_KeyboardRightGUI:
      return modifier_flag::right_command;
    case static_cast<uint32_t>(key_code::fn):
      return modifier_flag::fn;
    default:
      return modifier_flag::zero;
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

  static boost::optional<key_code> get_key_code(uint32_t usage_page, uint32_t usage) {
    switch (usage_page) {
    case kHIDPage_KeyboardOrKeypad:
      if (kHIDUsage_KeyboardErrorUndefined < usage && usage < kHIDUsage_Keyboard_Reserved) {
        return krbn::key_code(usage);
      }
      break;

    case kHIDPage_AppleVendorTopCase:
      if (usage == kHIDUsage_AV_TopCase_KeyboardFn) {
        return krbn::key_code::fn;
      }
      break;

    case kHIDPage_AppleVendorKeyboard:
      if (usage == kHIDUsage_AppleVendorKeyboard_Function) {
        return krbn::key_code::fn;
      }
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

    default:
      return pqrs::karabiner_virtual_hid_device::usage(key_code);
    }
  }

  static boost::optional<pointing_button> get_pointing_button(uint32_t usage_page, uint32_t usage) {
    if (usage_page == kHIDPage_Button) {
      return krbn::pointing_button(usage);
    }
    return boost::none;
  }

  static const std::unordered_map<std::string, keyboard_type>& get_keyboard_type_map(void) {
    static std::unordered_map<std::string, keyboard_type> map({
        {"none", keyboard_type::none},
        {"ansi", keyboard_type::ansi},
        {"iso", keyboard_type::iso},
        {"jis", keyboard_type::jis},
    });

    return map;
  }

  static boost::optional<keyboard_type> get_keyboard_type(const std::string& name) {
    auto& map = get_keyboard_type_map();
    auto it = map.find(name);
    if (it == map.end()) {
      return boost::none;
    }
    return it->second;
  }
};

struct operation_type_connect_struct {
  operation_type_connect_struct(void) : operation_type(operation_type::connect) {}

  const operation_type operation_type;
  connect_from connect_from;
  pid_t pid;
};

struct operation_type_system_preferences_values_updated_struct {
  operation_type_system_preferences_values_updated_struct(void) : operation_type(operation_type::system_preferences_values_updated) {}

  const operation_type operation_type;
  system_preferences::values values;
};

struct operation_type_clear_simple_modifications_struct {
  operation_type_clear_simple_modifications_struct(void) : operation_type(operation_type::clear_simple_modifications) {}

  const operation_type operation_type;
};

struct operation_type_add_simple_modification_struct {
  operation_type_add_simple_modification_struct(void) : operation_type(operation_type::add_simple_modification) {}

  const operation_type operation_type;
  key_code from_key_code;
  key_code to_key_code;
};

struct operation_type_clear_fn_function_keys_struct {
  operation_type_clear_fn_function_keys_struct(void) : operation_type(operation_type::clear_fn_function_keys) {}

  const operation_type operation_type;
};

struct operation_type_add_fn_function_key_struct {
  operation_type_add_fn_function_key_struct(void) : operation_type(operation_type::add_fn_function_key) {}

  const operation_type operation_type;
  key_code from_key_code;
  key_code to_key_code;
};

struct operation_type_virtual_hid_keyboard_configuration_updated_struct {
  operation_type_virtual_hid_keyboard_configuration_updated_struct(void) : operation_type(operation_type::virtual_hid_keyboard_configuration_updated) {}

  const operation_type operation_type;
  virtual_hid_keyboard_configuration_struct virtual_hid_keyboard_configuration_struct;
};

struct operation_type_clear_devices_struct {
  operation_type_clear_devices_struct(void) : operation_type(operation_type::clear_devices) {}

  const operation_type operation_type;
};

struct operation_type_add_device_struct {
  operation_type_add_device_struct(void) : operation_type(operation_type::add_device) {}

  const operation_type operation_type;
  device_identifiers_struct device_identifiers_struct;
  device_configuration_struct device_configuration_struct;
};

struct operation_type_complete_devices_struct {
  operation_type_complete_devices_struct(void) : operation_type(operation_type::complete_devices) {}

  const operation_type operation_type;
};
}
