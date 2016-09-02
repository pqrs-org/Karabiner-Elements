#pragma once

#include "boost_defs.hpp"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <boost/optional.hpp>
#include <string>
#include <unordered_map>

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

  extra_ = 0x1000,
  // static virtual key codes
  // (Do not change their values since these virtual key codes may be in user preferences.)
  vk_none = 0x1001,
  vk_fn_modifier = 0x1002,

  // virtual key codes
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
      // A-Z
      map["a"] = key_code(kHIDUsage_KeyboardA);
      map["b"] = key_code(kHIDUsage_KeyboardB);
      map["c"] = key_code(kHIDUsage_KeyboardC);
      map["d"] = key_code(kHIDUsage_KeyboardD);
      map["e"] = key_code(kHIDUsage_KeyboardE);
      map["f"] = key_code(kHIDUsage_KeyboardF);
      map["g"] = key_code(kHIDUsage_KeyboardG);
      map["h"] = key_code(kHIDUsage_KeyboardH);
      map["i"] = key_code(kHIDUsage_KeyboardI);
      map["j"] = key_code(kHIDUsage_KeyboardJ);
      map["k"] = key_code(kHIDUsage_KeyboardK);
      map["l"] = key_code(kHIDUsage_KeyboardL);
      map["m"] = key_code(kHIDUsage_KeyboardM);
      map["n"] = key_code(kHIDUsage_KeyboardN);
      map["o"] = key_code(kHIDUsage_KeyboardO);
      map["p"] = key_code(kHIDUsage_KeyboardP);
      map["q"] = key_code(kHIDUsage_KeyboardQ);
      map["r"] = key_code(kHIDUsage_KeyboardR);
      map["s"] = key_code(kHIDUsage_KeyboardS);
      map["t"] = key_code(kHIDUsage_KeyboardT);
      map["u"] = key_code(kHIDUsage_KeyboardU);
      map["v"] = key_code(kHIDUsage_KeyboardV);
      map["w"] = key_code(kHIDUsage_KeyboardW);
      map["x"] = key_code(kHIDUsage_KeyboardX);
      map["y"] = key_code(kHIDUsage_KeyboardY);
      map["z"] = key_code(kHIDUsage_KeyboardZ);
      // 1-0
      map["1"] = key_code(kHIDUsage_Keyboard1);
      map["2"] = key_code(kHIDUsage_Keyboard2);
      map["3"] = key_code(kHIDUsage_Keyboard3);
      map["4"] = key_code(kHIDUsage_Keyboard4);
      map["5"] = key_code(kHIDUsage_Keyboard5);
      map["6"] = key_code(kHIDUsage_Keyboard6);
      map["7"] = key_code(kHIDUsage_Keyboard7);
      map["8"] = key_code(kHIDUsage_Keyboard8);
      map["9"] = key_code(kHIDUsage_Keyboard9);
      map["0"] = key_code(kHIDUsage_Keyboard0);
      //
      map["return_or_enter"] = key_code(kHIDUsage_KeyboardReturnOrEnter);
      map["escape"] = key_code(kHIDUsage_KeyboardEscape);
      map["delete_or_backspace"] = key_code(kHIDUsage_KeyboardDeleteOrBackspace);
      map["tab"] = key_code(kHIDUsage_KeyboardTab);
      map["spacebar"] = key_code(kHIDUsage_KeyboardSpacebar);
      map["hyphen"] = key_code(kHIDUsage_KeyboardHyphen);
      map["equal_sign"] = key_code(kHIDUsage_KeyboardEqualSign);
      map["open_bracket"] = key_code(kHIDUsage_KeyboardOpenBracket);
      map["close_bracket"] = key_code(kHIDUsage_KeyboardCloseBracket);
      map["backslash"] = key_code(kHIDUsage_KeyboardBackslash);
      map["non_us_pound"] = key_code(kHIDUsage_KeyboardNonUSPound);
      map["semicolon"] = key_code(kHIDUsage_KeyboardSemicolon);
      map["quote"] = key_code(kHIDUsage_KeyboardQuote);
      map["grave_accent_and_tilde"] = key_code(kHIDUsage_KeyboardGraveAccentAndTilde);
      map["comma"] = key_code(kHIDUsage_KeyboardComma);
      map["period"] = key_code(kHIDUsage_KeyboardPeriod);
      map["slash"] = key_code(kHIDUsage_KeyboardSlash);
      map["caps_lock"] = key_code(kHIDUsage_KeyboardCapsLock);
      // f1-f12
      map["f1"] = key_code(kHIDUsage_KeyboardF1);
      map["f2"] = key_code(kHIDUsage_KeyboardF2);
      map["f3"] = key_code(kHIDUsage_KeyboardF3);
      map["f4"] = key_code(kHIDUsage_KeyboardF4);
      map["f5"] = key_code(kHIDUsage_KeyboardF5);
      map["f6"] = key_code(kHIDUsage_KeyboardF6);
      map["f7"] = key_code(kHIDUsage_KeyboardF7);
      map["f8"] = key_code(kHIDUsage_KeyboardF8);
      map["f9"] = key_code(kHIDUsage_KeyboardF9);
      map["f10"] = key_code(kHIDUsage_KeyboardF10);
      map["f11"] = key_code(kHIDUsage_KeyboardF11);
      map["f12"] = key_code(kHIDUsage_KeyboardF12);
      //
      map["print_screen"] = key_code(kHIDUsage_KeyboardPrintScreen);
      map["scroll_lock"] = key_code(kHIDUsage_KeyboardScrollLock);
      map["pause"] = key_code(kHIDUsage_KeyboardPause);
      map["insert"] = key_code(kHIDUsage_KeyboardInsert);
      map["home"] = key_code(kHIDUsage_KeyboardHome);
      map["page_up"] = key_code(kHIDUsage_KeyboardPageUp);
      map["delete_forward"] = key_code(kHIDUsage_KeyboardDeleteForward);
      map["end"] = key_code(kHIDUsage_KeyboardEnd);
      map["page_down"] = key_code(kHIDUsage_KeyboardPageDown);
      map["right_arrow"] = key_code(kHIDUsage_KeyboardRightArrow);
      map["left_arrow"] = key_code(kHIDUsage_KeyboardLeftArrow);
      map["down_arrow"] = key_code(kHIDUsage_KeyboardDownArrow);
      map["up_arrow"] = key_code(kHIDUsage_KeyboardUpArrow);
      // keypad
      map["keypad_num_lock"] = key_code(kHIDUsage_KeypadNumLock);
      map["keypad_slash"] = key_code(kHIDUsage_KeypadSlash);
      map["keypad_asterisk"] = key_code(kHIDUsage_KeypadAsterisk);
      map["keypad_hyphen"] = key_code(kHIDUsage_KeypadHyphen);
      map["keypad_plus"] = key_code(kHIDUsage_KeypadPlus);
      map["keypad_enter"] = key_code(kHIDUsage_KeypadEnter);
      map["keypad_1"] = key_code(kHIDUsage_Keypad1);
      map["keypad_2"] = key_code(kHIDUsage_Keypad2);
      map["keypad_3"] = key_code(kHIDUsage_Keypad3);
      map["keypad_4"] = key_code(kHIDUsage_Keypad4);
      map["keypad_5"] = key_code(kHIDUsage_Keypad5);
      map["keypad_6"] = key_code(kHIDUsage_Keypad6);
      map["keypad_7"] = key_code(kHIDUsage_Keypad7);
      map["keypad_8"] = key_code(kHIDUsage_Keypad8);
      map["keypad_9"] = key_code(kHIDUsage_Keypad9);
      map["keypad_0"] = key_code(kHIDUsage_Keypad0);
      map["keypad_period"] = key_code(kHIDUsage_KeypadPeriod);
      //
      map["non_us_backslash"] = key_code(kHIDUsage_KeyboardNonUSBackslash);
      map["application"] = key_code(kHIDUsage_KeyboardApplication);
      map["power"] = key_code(kHIDUsage_KeyboardPower);
      // keypad
      map["keypad_equal_sign"] = key_code(kHIDUsage_KeypadEqualSign);
      // f13-f24
      map["f13"] = key_code(kHIDUsage_KeyboardF13);
      map["f14"] = key_code(kHIDUsage_KeyboardF14);
      map["f15"] = key_code(kHIDUsage_KeyboardF15);
      map["f16"] = key_code(kHIDUsage_KeyboardF16);
      map["f17"] = key_code(kHIDUsage_KeyboardF17);
      map["f18"] = key_code(kHIDUsage_KeyboardF18);
      map["f19"] = key_code(kHIDUsage_KeyboardF19);
      map["f20"] = key_code(kHIDUsage_KeyboardF20);
      map["f21"] = key_code(kHIDUsage_KeyboardF21);
      map["f22"] = key_code(kHIDUsage_KeyboardF22);
      map["f23"] = key_code(kHIDUsage_KeyboardF23);
      map["f24"] = key_code(kHIDUsage_KeyboardF24);
      //
      map["execute"] = key_code(kHIDUsage_KeyboardExecute);
      map["help"] = key_code(kHIDUsage_KeyboardHelp);
      map["menu"] = key_code(kHIDUsage_KeyboardMenu);
      map["select"] = key_code(kHIDUsage_KeyboardSelect);
      map["stop"] = key_code(kHIDUsage_KeyboardStop);
      map["again"] = key_code(kHIDUsage_KeyboardAgain);
      map["undo"] = key_code(kHIDUsage_KeyboardUndo);
      map["cut"] = key_code(kHIDUsage_KeyboardCut);
      map["copy"] = key_code(kHIDUsage_KeyboardCopy);
      map["paste"] = key_code(kHIDUsage_KeyboardPaste);
      map["find"] = key_code(kHIDUsage_KeyboardFind);
      // volume controls
      map["mute"] = key_code(kHIDUsage_KeyboardMute);
      map["volume_up"] = key_code(kHIDUsage_KeyboardVolumeUp);
      map["volume_down"] = key_code(kHIDUsage_KeyboardVolumeDown);
      // locking
      map["locking_caps_lock"] = key_code(kHIDUsage_KeyboardLockingCapsLock);
      map["locking_num_lock"] = key_code(kHIDUsage_KeyboardLockingNumLock);
      map["locking_scroll_lock"] = key_code(kHIDUsage_KeyboardLockingScrollLock);
      // keypad
      map["keypad_comma"] = key_code(kHIDUsage_KeypadComma);
      map["keypad_equal_sign_as400"] = key_code(kHIDUsage_KeypadEqualSignAS400);
      // international
      map["international1"] = key_code(kHIDUsage_KeyboardInternational1);
      map["international2"] = key_code(kHIDUsage_KeyboardInternational2);
      map["international3"] = key_code(kHIDUsage_KeyboardInternational3);
      map["international4"] = key_code(kHIDUsage_KeyboardInternational4);
      map["international5"] = key_code(kHIDUsage_KeyboardInternational5);
      map["international6"] = key_code(kHIDUsage_KeyboardInternational6);
      map["international7"] = key_code(kHIDUsage_KeyboardInternational7);
      map["international8"] = key_code(kHIDUsage_KeyboardInternational8);
      map["international9"] = key_code(kHIDUsage_KeyboardInternational9);
      // lang
      map["lang1"] = key_code(kHIDUsage_KeyboardLANG1);
      map["lang2"] = key_code(kHIDUsage_KeyboardLANG2);
      map["lang3"] = key_code(kHIDUsage_KeyboardLANG3);
      map["lang4"] = key_code(kHIDUsage_KeyboardLANG4);
      map["lang5"] = key_code(kHIDUsage_KeyboardLANG5);
      map["lang6"] = key_code(kHIDUsage_KeyboardLANG6);
      map["lang7"] = key_code(kHIDUsage_KeyboardLANG7);
      map["lang8"] = key_code(kHIDUsage_KeyboardLANG8);
      map["lang9"] = key_code(kHIDUsage_KeyboardLANG9);
      //
      map["alternate_erase"] = key_code(kHIDUsage_KeyboardAlternateErase);
      map["sys_req_or_attention"] = key_code(kHIDUsage_KeyboardSysReqOrAttention);
      map["cancel"] = key_code(kHIDUsage_KeyboardCancel);
      map["clear"] = key_code(kHIDUsage_KeyboardClear);
      map["prior"] = key_code(kHIDUsage_KeyboardPrior);
      map["return"] = key_code(kHIDUsage_KeyboardReturn);
      map["separator"] = key_code(kHIDUsage_KeyboardSeparator);
      map["out"] = key_code(kHIDUsage_KeyboardOut);
      map["oper"] = key_code(kHIDUsage_KeyboardOper);
      map["clear_or_again"] = key_code(kHIDUsage_KeyboardClearOrAgain);
      map["cr_sel_or_props"] = key_code(kHIDUsage_KeyboardCrSelOrProps);
      map["ex_sel"] = key_code(kHIDUsage_KeyboardExSel);
      // modifiers
      map["left_control"] = key_code(kHIDUsage_KeyboardLeftControl);
      map["left_shift"] = key_code(kHIDUsage_KeyboardLeftShift);
      map["left_alt"] = key_code(kHIDUsage_KeyboardLeftAlt);
      map["left_gui"] = key_code(kHIDUsage_KeyboardLeftGUI);
      map["right_control"] = key_code(kHIDUsage_KeyboardRightControl);
      map["right_shift"] = key_code(kHIDUsage_KeyboardRightShift);
      map["right_alt"] = key_code(kHIDUsage_KeyboardRightAlt);
      map["right_gui"] = key_code(kHIDUsage_KeyboardRightGUI);
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
