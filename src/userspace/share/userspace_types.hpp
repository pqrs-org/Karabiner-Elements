#pragma once

#include "boost_defs.hpp"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include <IOKit/hidsystem/IOHIDShared.h>
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
  clear_simple_modifications,
  add_simple_modification,
  // grabber -> console_user_server
  stop_key_repeat,
  post_modifier_flags,
  post_key,
};

enum class event_type : uint32_t {
  key_down,
  key_up,
};

enum class key_code : uint32_t {
  // 0x00 - 0xff is usage page
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

  extra_ = 0x1000,
  // static virtual key codes
  // (Do not change their values since these virtual key codes may be in user preferences.)
  vk_none = 0x1001,
  vk_fn_modifier = 0x1002,

  // virtual key codes
  vk_function_keys_start_,
  vk_f1,
  vk_f2,
  vk_f3,
  vk_f4,
  vk_f5,
  vk_f6,
  vk_f7,
  vk_f8,
  vk_f9,
  vk_f10,
  vk_f11,
  vk_f12,
  vk_fn_f1,
  vk_fn_f2,
  vk_fn_f3,
  vk_fn_f4,
  vk_fn_f5,
  vk_fn_f6,
  vk_fn_f7,
  vk_fn_f8,
  vk_fn_f9,
  vk_fn_f10,
  vk_fn_f11,
  vk_fn_f12,
  vk_consumer_brightness_down,
  vk_consumer_brightness_up,
  vk_consumer_illumination_down,
  vk_consumer_illumination_up,
  vk_consumer_mute,
  vk_consumer_next,
  vk_consumer_play,
  vk_consumer_previous,
  vk_consumer_sound_down,
  vk_consumer_sound_up,
  vk_dashboard,
  vk_launchpad,
  vk_mission_control,
  vk_function_keys_end_,
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
  none,
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

          // Extra
          {"fn", key_code::vk_fn_modifier},
          {"vk_consumer_brightness_down", key_code::vk_consumer_brightness_down},
          {"vk_consumer_brightness_up", key_code::vk_consumer_brightness_up},
          {"vk_consumer_illumination_down", key_code::vk_consumer_illumination_down},
          {"vk_consumer_illumination_up", key_code::vk_consumer_illumination_up},
          {"vk_consumer_mute", key_code::vk_consumer_mute},
          {"vk_consumer_next", key_code::vk_consumer_next},
          {"vk_consumer_play", key_code::vk_consumer_play},
          {"vk_consumer_previous", key_code::vk_consumer_previous},
          {"vk_consumer_sound_down", key_code::vk_consumer_sound_down},
          {"vk_consumer_sound_up", key_code::vk_consumer_sound_up},
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
};

struct operation_type_connect_struct {
  operation_type_connect_struct(void) : operation_type(operation_type::connect) {}

  const operation_type operation_type;
  pid_t console_user_server_pid;
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

struct operation_type_stop_key_repeat_struct {
  operation_type_stop_key_repeat_struct(void) : operation_type(operation_type::stop_key_repeat) {}

  const operation_type operation_type;
};

struct operation_type_post_modifier_flags_struct {
  operation_type_post_modifier_flags_struct(void) : operation_type(operation_type::post_modifier_flags) {}

  const operation_type operation_type;
  IOOptionBits flags;
};

struct operation_type_post_key_struct {
  operation_type_post_key_struct(void) : operation_type(operation_type::post_key) {}

  const operation_type operation_type;
  key_code key_code;
  event_type event_type;
  IOOptionBits flags;
};
}
