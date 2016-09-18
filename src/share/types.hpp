#pragma once

#include "boost_defs.hpp"

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
  // event_dispatcher,console_user_server -> grabber
  connect,
  // grabber -> event_dispatcher,console_user_server
  connect_ack,
  // console_user_server -> grabber
  system_preferences_values_updated,
  clear_simple_modifications,
  add_simple_modification,
  // grabber -> console_user_server
  stop_key_repeat,
  // event_dispatcher -> grabber
  set_caps_lock_led_state,
  // grabber -> event_dispatcher
  post_modifier_flags,
  toggle_caps_lock_state,
  set_caps_lock_state,
  refresh_caps_lock_led,
  post_key,
};

enum class connect_from : uint8_t {
  event_dispatcher,
  console_user_server,
};

enum class event_type : uint32_t {
  key_down,
  key_up,
};

enum class key_code : uint32_t {
  // 0x00 - 0xff are usage pages
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

  right_arrow = kHIDUsage_KeyboardRightArrow,
  left_arrow = kHIDUsage_KeyboardLeftArrow,
  down_arrow = kHIDUsage_KeyboardDownArrow,
  up_arrow = kHIDUsage_KeyboardUpArrow,

  home = kHIDUsage_KeyboardHome,
  page_up = kHIDUsage_KeyboardPageUp,
  delete_forward = kHIDUsage_KeyboardDeleteForward,
  end = kHIDUsage_KeyboardEnd,
  page_down = kHIDUsage_KeyboardPageDown,
  keypad_enter = kHIDUsage_KeypadEnter,

  mute = kHIDUsage_KeyboardMute,
  volume_up = kHIDUsage_KeyboardVolumeUp,
  volume_down = kHIDUsage_KeyboardVolumeDown,

  // 0x1000 - are karabiner own virtual key codes
  extra_ = 0x1000,
  // predefined virtual modifier flags
  vk_none,
  vk_fn_modifier,

  // virtual key codes
  vk_consumer_brightness_down,
  vk_consumer_brightness_up,
  vk_consumer_illumination_down,
  vk_consumer_illumination_up,
  vk_consumer_next,
  vk_consumer_play,
  vk_consumer_previous,
  vk_dashboard,
  vk_launchpad,
  vk_mission_control,
};

enum class pointing_button : uint32_t {
};

enum class modifier_flag : uint32_t {
  zero,
  none,
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
    case static_cast<uint32_t>(key_code::vk_fn_modifier):
      return modifier_flag::fn;
    default:
      return modifier_flag::zero;
    }
  }

  // string -> hid usage map
  static const std::unordered_map<std::string, key_code>& get_key_code_map(void) {
    static std::unordered_map<std::string, key_code> map;

    if (map.empty()) {
      std::vector<std::pair<std::string, key_code>> pairs{
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
          {"volume_up", key_code(kHIDUsage_KeyboardVolumeUp)},
          {"volume_down", key_code(kHIDUsage_KeyboardVolumeDown)},
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

          // Extra
          {"fn", key_code::vk_fn_modifier},
          {"vk_consumer_brightness_down", key_code::vk_consumer_brightness_down},
          {"vk_consumer_brightness_up", key_code::vk_consumer_brightness_up},
          {"vk_consumer_illumination_down", key_code::vk_consumer_illumination_down},
          {"vk_consumer_illumination_up", key_code::vk_consumer_illumination_up},
          {"vk_consumer_next", key_code::vk_consumer_next},
          {"vk_consumer_play", key_code::vk_consumer_play},
          {"vk_consumer_previous", key_code::vk_consumer_previous},
          {"vk_dashboard", key_code::vk_dashboard},
          {"vk_launchpad", key_code::vk_launchpad},
          {"vk_mission_control", key_code::vk_mission_control},
      };
      for (const auto& pair : pairs) {
        if (map.find(pair.first) != map.end()) {
          std::cerr << "fatal: duplicated key: " << pair.first << std::endl;
          exit(1);
        } else {
          map[pair.first] = pair.second;
        }
      }
    }
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

  // hid usage -> mac key code
  static const std::unordered_map<key_code, uint8_t>& get_mac_key_map(void) {
    static std::unordered_map<key_code, uint8_t> map;
    if (map.empty()) {
      map[key_code(kHIDUsage_KeyboardA)] = 0x0;
      map[key_code(kHIDUsage_KeyboardB)] = 0xb;
      map[key_code(kHIDUsage_KeyboardC)] = 0x8;
      map[key_code(kHIDUsage_KeyboardD)] = 0x2;
      map[key_code(kHIDUsage_KeyboardE)] = 0xe;
      map[key_code(kHIDUsage_KeyboardF)] = 0x3;
      map[key_code(kHIDUsage_KeyboardG)] = 0x5;
      map[key_code(kHIDUsage_KeyboardH)] = 0x4;
      map[key_code(kHIDUsage_KeyboardI)] = 0x22;
      map[key_code(kHIDUsage_KeyboardJ)] = 0x26;
      map[key_code(kHIDUsage_KeyboardK)] = 0x28;
      map[key_code(kHIDUsage_KeyboardL)] = 0x25;
      map[key_code(kHIDUsage_KeyboardM)] = 0x2e;
      map[key_code(kHIDUsage_KeyboardN)] = 0x2d;
      map[key_code(kHIDUsage_KeyboardO)] = 0x1f;
      map[key_code(kHIDUsage_KeyboardP)] = 0x23;
      map[key_code(kHIDUsage_KeyboardQ)] = 0xc;
      map[key_code(kHIDUsage_KeyboardR)] = 0xf;
      map[key_code(kHIDUsage_KeyboardS)] = 0x1;
      map[key_code(kHIDUsage_KeyboardT)] = 0x11;
      map[key_code(kHIDUsage_KeyboardU)] = 0x20;
      map[key_code(kHIDUsage_KeyboardV)] = 0x9;
      map[key_code(kHIDUsage_KeyboardW)] = 0xd;
      map[key_code(kHIDUsage_KeyboardX)] = 0x7;
      map[key_code(kHIDUsage_KeyboardY)] = 0x10;
      map[key_code(kHIDUsage_KeyboardZ)] = 0x6;

      map[key_code(kHIDUsage_Keyboard1)] = 0x12;
      map[key_code(kHIDUsage_Keyboard2)] = 0x13;
      map[key_code(kHIDUsage_Keyboard3)] = 0x14;
      map[key_code(kHIDUsage_Keyboard4)] = 0x15;
      map[key_code(kHIDUsage_Keyboard5)] = 0x17;
      map[key_code(kHIDUsage_Keyboard6)] = 0x16;
      map[key_code(kHIDUsage_Keyboard7)] = 0x1a;
      map[key_code(kHIDUsage_Keyboard8)] = 0x1c;
      map[key_code(kHIDUsage_Keyboard9)] = 0x19;
      map[key_code(kHIDUsage_Keyboard0)] = 0x1d;

      map[key_code(kHIDUsage_KeyboardReturnOrEnter)] = 0x24;
      map[key_code(kHIDUsage_KeyboardEscape)] = 0x35;
      map[key_code(kHIDUsage_KeyboardDeleteOrBackspace)] = 0x33;
      map[key_code(kHIDUsage_KeyboardTab)] = 0x30;
      map[key_code(kHIDUsage_KeyboardSpacebar)] = 0x31;
      map[key_code(kHIDUsage_KeyboardHyphen)] = 0x1b;
      map[key_code(kHIDUsage_KeyboardEqualSign)] = 0x18;
      map[key_code(kHIDUsage_KeyboardOpenBracket)] = 0x21;
      map[key_code(kHIDUsage_KeyboardCloseBracket)] = 0x1e;
      map[key_code(kHIDUsage_KeyboardBackslash)] = 0x2a;
      map[key_code(kHIDUsage_KeyboardNonUSPound)] = 0x2a; // == kHIDUsage_KeyboardBackslash
      map[key_code(kHIDUsage_KeyboardSemicolon)] = 0x29;
      map[key_code(kHIDUsage_KeyboardQuote)] = 0x27;
      map[key_code(kHIDUsage_KeyboardGraveAccentAndTilde)] = 0x32;
      map[key_code(kHIDUsage_KeyboardComma)] = 0x2b;
      map[key_code(kHIDUsage_KeyboardPeriod)] = 0x2f;
      map[key_code(kHIDUsage_KeyboardSlash)] = 0x2c;
      map[key_code(kHIDUsage_KeyboardCapsLock)] = 0x39;

      map[key_code(kHIDUsage_KeyboardF1)] = 0x7a;
      map[key_code(kHIDUsage_KeyboardF2)] = 0x78;
      map[key_code(kHIDUsage_KeyboardF3)] = 0x63;
      map[key_code(kHIDUsage_KeyboardF4)] = 0x76;
      map[key_code(kHIDUsage_KeyboardF5)] = 0x60;
      map[key_code(kHIDUsage_KeyboardF6)] = 0x61;
      map[key_code(kHIDUsage_KeyboardF7)] = 0x62;
      map[key_code(kHIDUsage_KeyboardF8)] = 0x64;
      map[key_code(kHIDUsage_KeyboardF9)] = 0x65;
      map[key_code(kHIDUsage_KeyboardF10)] = 0x6d;
      map[key_code(kHIDUsage_KeyboardF11)] = 0x67;
      map[key_code(kHIDUsage_KeyboardF12)] = 0x6f;

      map[key_code(kHIDUsage_KeyboardPrintScreen)] = 0x69;
      map[key_code(kHIDUsage_KeyboardScrollLock)] = 0x6b;

      map[key_code(kHIDUsage_KeyboardPause)] = 0x71;
      map[key_code(kHIDUsage_KeyboardInsert)] = 0x72;
      map[key_code(kHIDUsage_KeyboardHome)] = 0x73;
      map[key_code(kHIDUsage_KeyboardPageUp)] = 0x74;
      map[key_code(kHIDUsage_KeyboardDeleteForward)] = 0x75;
      map[key_code(kHIDUsage_KeyboardEnd)] = 0x77;
      map[key_code(kHIDUsage_KeyboardPageDown)] = 0x79;
      map[key_code(kHIDUsage_KeyboardRightArrow)] = 0x7c;
      map[key_code(kHIDUsage_KeyboardLeftArrow)] = 0x7b;
      map[key_code(kHIDUsage_KeyboardDownArrow)] = 0x7d;
      map[key_code(kHIDUsage_KeyboardUpArrow)] = 0x7e;

      map[key_code(kHIDUsage_KeypadNumLock)] = 0x47;
      map[key_code(kHIDUsage_KeypadSlash)] = 0x4b;
      map[key_code(kHIDUsage_KeypadAsterisk)] = 0x43;
      map[key_code(kHIDUsage_KeypadHyphen)] = 0x4e;
      map[key_code(kHIDUsage_KeypadPlus)] = 0x45;
      map[key_code(kHIDUsage_KeypadEnter)] = 0x4c;
      map[key_code(kHIDUsage_Keypad1)] = 0x53;
      map[key_code(kHIDUsage_Keypad2)] = 0x54;
      map[key_code(kHIDUsage_Keypad3)] = 0x55;
      map[key_code(kHIDUsage_Keypad4)] = 0x56;
      map[key_code(kHIDUsage_Keypad5)] = 0x57;
      map[key_code(kHIDUsage_Keypad6)] = 0x58;
      map[key_code(kHIDUsage_Keypad7)] = 0x59;
      map[key_code(kHIDUsage_Keypad8)] = 0x5b;
      map[key_code(kHIDUsage_Keypad9)] = 0x5c;
      map[key_code(kHIDUsage_Keypad0)] = 0x52;
      map[key_code(kHIDUsage_KeypadPeriod)] = 0x41;

      map[key_code(kHIDUsage_KeyboardNonUSBackslash)] = 0xa;
      map[key_code(kHIDUsage_KeyboardApplication)] = 0x6e;

      // map[key_code(kHIDUsage_KeyboardPower)] => get_mac_aux_control_button_map

      map[key_code(kHIDUsage_KeypadEqualSign)] = 0x51;

      map[key_code(kHIDUsage_KeyboardF13)] = 0x69;
      map[key_code(kHIDUsage_KeyboardF14)] = 0x6b;
      map[key_code(kHIDUsage_KeyboardF15)] = 0x71;
      map[key_code(kHIDUsage_KeyboardF16)] = 0x6a;
      map[key_code(kHIDUsage_KeyboardF17)] = 0x40;
      map[key_code(kHIDUsage_KeyboardF18)] = 0x4f;
      map[key_code(kHIDUsage_KeyboardF19)] = 0x50;
      map[key_code(kHIDUsage_KeyboardF20)] = 0x5a;
      // map[key_code(kHIDUsage_KeyboardF21)] => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardF22)] => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardF23)] => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardF24)] => mac ignores this key

      // map[key_code(kHIDUsage_KeyboardExecute)] => mac ignores this key
      map[key_code(kHIDUsage_KeyboardHelp)] = 0x72;
      // map[key_code(kHIDUsage_KeyboardMenu)] = mac ignores this key
      // map[key_code(kHIDUsage_KeyboardSelect)] = mac ignores this key
      // map[key_code(kHIDUsage_KeyboardStop)] = mac ignores this key
      // map[key_code(kHIDUsage_KeyboardAgain)] = mac ignores this key
      // map[key_code(kHIDUsage_KeyboardUndo)] = mac ignores this key
      // map[key_code(kHIDUsage_KeyboardCut)] = mac ignores this key
      // map[key_code(kHIDUsage_KeyboardCopy)] = mac ignores this key
      // map[key_code(kHIDUsage_KeyboardPaste)] = mac ignores this key
      // map[key_code(kHIDUsage_KeyboardFind)] = mac ignores this key
      // map[key_code(kHIDUsage_KeyboardMute)] => get_mac_aux_control_button_map
      // map[key_code(kHIDUsage_KeyboardVolumeUp)] => get_mac_aux_control_button_map
      // map[key_code(kHIDUsage_KeyboardVolumeDown)] => get_mac_aux_control_button_map

      // map[key_code(kHIDUsage_KeyboardLockingCapsLock)] => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardLockingNumLock)] => ??
      // map[key_code(kHIDUsage_KeyboardLockingScrollLock)] => mac ignores this key
      map[key_code(kHIDUsage_KeypadComma)] = 0x5f;
      // map[key_code(kHIDUsage_KeypadEqualSignAS400)] => mac ignores this key

      map[key_code(kHIDUsage_KeyboardInternational1)] = 0x5e;
      // map[key_code(kHIDUsage_KeyboardInternational2)] => mac ignores this key
      map[key_code(kHIDUsage_KeyboardInternational3)] = 0x5d;
      // map[key_code(kHIDUsage_KeyboardInternational4)] => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardInternational5)] => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardInternational6)] => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardInternational7)] => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardInternational8)] => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardInternational9)] => mac ignores this key

      map[key_code(kHIDUsage_KeyboardLANG1)] = 0x68;
      map[key_code(kHIDUsage_KeyboardLANG2)] = 0x66;
      // map[key_code(kHIDUsage_KeyboardLANG3)] => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardLANG4)] => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardLANG5)] => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardLANG6)] => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardLANG7)] => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardLANG8)] => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardLANG9)] => mac ignores this key

      // map[key_code(kHIDUsage_KeyboardAlternateErase)]    => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardSysReqOrAttention)] => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardCancel)]            => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardClear)]             => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardPrior)]             => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardReturn)]            => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardSeparator)]         => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardOut)]               => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardOper)]              => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardClearOrAgain)]      => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardCrSelOrProps)]      => mac ignores this key
      // map[key_code(kHIDUsage_KeyboardExSel)]             => mac ignores this key

      map[key_code(kHIDUsage_KeyboardLeftControl)] = 0x3b;
      map[key_code(kHIDUsage_KeyboardLeftShift)] = 0x38;
      map[key_code(kHIDUsage_KeyboardLeftAlt)] = 0x3a;
      map[key_code(kHIDUsage_KeyboardLeftGUI)] = 0x37;
      map[key_code(kHIDUsage_KeyboardRightControl)] = 0x3e;
      map[key_code(kHIDUsage_KeyboardRightShift)] = 0x3c;
      map[key_code(kHIDUsage_KeyboardRightAlt)] = 0x3d;
      map[key_code(kHIDUsage_KeyboardRightGUI)] = 0x36;

      map[key_code::vk_fn_modifier] = 0x3f;
      map[key_code::vk_dashboard] = 0x82;
      map[key_code::vk_launchpad] = 0x83;
      map[key_code::vk_mission_control] = 0xa0;
    }
    return map;
  }

  static boost::optional<uint8_t> get_mac_key(key_code key_code) {
    auto& map = get_mac_key_map();
    auto it = map.find(key_code);
    if (it == map.end()) {
      return boost::none;
    }
    return it->second;
  }

  static const std::unordered_map<key_code, uint8_t>& get_mac_aux_control_button_map(void) {
    static std::unordered_map<key_code, uint8_t> map;
    if (map.empty()) {
      map[key_code(kHIDUsage_KeyboardPower)] = NX_POWER_KEY;
      map[key_code(kHIDUsage_KeyboardMute)] = NX_KEYTYPE_MUTE;
      map[key_code(kHIDUsage_KeyboardVolumeUp)] = NX_KEYTYPE_SOUND_UP;
      map[key_code(kHIDUsage_KeyboardVolumeDown)] = NX_KEYTYPE_SOUND_DOWN;

      map[key_code::vk_consumer_brightness_down] = NX_KEYTYPE_BRIGHTNESS_DOWN;
      map[key_code::vk_consumer_brightness_up] = NX_KEYTYPE_BRIGHTNESS_UP;
      map[key_code::vk_consumer_illumination_down] = NX_KEYTYPE_ILLUMINATION_DOWN;
      map[key_code::vk_consumer_illumination_up] = NX_KEYTYPE_ILLUMINATION_UP;
      map[key_code::vk_consumer_next] = NX_KEYTYPE_NEXT;
      map[key_code::vk_consumer_play] = NX_KEYTYPE_PLAY;
      map[key_code::vk_consumer_previous] = NX_KEYTYPE_PREVIOUS;
    }
    return map;
  }

  static boost::optional<uint8_t> get_mac_aux_control_button(key_code key_code) {
    auto& map = get_mac_aux_control_button_map();
    auto it = map.find(key_code);
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

struct operation_type_connect_ack_struct {
  operation_type_connect_ack_struct(void) : operation_type(operation_type::connect_ack) {}

  const operation_type operation_type;
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

struct operation_type_set_caps_lock_led_state_struct {
  operation_type_set_caps_lock_led_state_struct(void) : operation_type(operation_type::set_caps_lock_led_state) {}

  const operation_type operation_type;
  led_state led_state;
};

struct operation_type_stop_key_repeat_struct {
  operation_type_stop_key_repeat_struct(void) : operation_type(operation_type::stop_key_repeat) {}

  const operation_type operation_type;
};

struct operation_type_post_modifier_flags_struct {
  operation_type_post_modifier_flags_struct(void) : operation_type(operation_type::post_modifier_flags) {}

  const operation_type operation_type;
  key_code key_code;
  IOOptionBits flags;
};

struct operation_type_toggle_caps_lock_state_struct {
  operation_type_toggle_caps_lock_state_struct(void) : operation_type(operation_type::toggle_caps_lock_state) {}

  const operation_type operation_type;
};

struct operation_type_set_caps_lock_state_struct {
  operation_type_set_caps_lock_state_struct(void) : operation_type(operation_type::set_caps_lock_state) {}

  const operation_type operation_type;
  bool state;
};

struct operation_type_refresh_caps_lock_led_struct {
  operation_type_refresh_caps_lock_led_struct(void) : operation_type(operation_type::refresh_caps_lock_led) {}

  const operation_type operation_type;
};

struct operation_type_post_key_struct {
  operation_type_post_key_struct(void) : operation_type(operation_type::post_key) {}

  const operation_type operation_type;
  key_code key_code;
  event_type event_type;
  IOOptionBits flags;
  bool repeat;
};
}
