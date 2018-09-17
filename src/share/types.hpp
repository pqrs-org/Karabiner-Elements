#pragma once

#include "boost_defs.hpp"

#include "Karabiner-VirtualHIDDevice/dist/include/karabiner_virtual_hid_device_methods.hpp"
#include "apple_hid_usage_tables.hpp"
#include "constants.hpp"
#include "input_source_utility.hpp"
#include "json_utility.hpp"
#include "logger.hpp"
#include "stream_utility.hpp"
#include "types/absolute_time.hpp"
#include "types/consumer_key_code.hpp"
#include "types/device_identifiers.hpp"
#include "types/event_type.hpp"
#include "types/grabbable_state.hpp"
#include "types/hid_usage.hpp"
#include "types/hid_usage_page.hpp"
#include "types/hid_value.hpp"
#include "types/input_source_identifiers.hpp"
#include "types/input_source_selector.hpp"
#include "types/key_code.hpp"
#include "types/led_state.hpp"
#include "types/location_id.hpp"
#include "types/modifier_flag.hpp"
#include "types/mouse_key.hpp"
#include "types/pointing_button.hpp"
#include "types/pointing_motion.hpp"
#include "types/product_id.hpp"
#include "types/registry_entry_id.hpp"
#include "types/vendor_id.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <IOKit/hidsystem/ev_keymap.h>
#include <boost/functional/hash.hpp>
#include <boost/optional.hpp>
#include <cstring>
#include <iostream>
#include <json/json.hpp>
#include <string>
#include <thread>
#include <type_safe/strong_typedef.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace krbn {
class device_detail;

enum class operation_type : uint8_t {
  none,
  // observer -> grabber
  grabbable_state_changed,
  // console_user_server -> grabber
  connect_console_user_server,
  system_preferences_updated,
  frontmost_application_changed,
  input_source_changed,
  // grabber -> console_user_server
  shell_command_execution,
  select_input_source,
};

enum class device_id : uint32_t {
  zero = 0,
};

enum class pointing_event : uint32_t {
  button,
  x,
  y,
  vertical_wheel,
  horizontal_wheel,
};

class system_preferences final {
public:
  system_preferences(void) : keyboard_fn_state_(false),
                             swipe_scroll_direction_(true),
                             keyboard_type_(40) {
  }

  bool get_keyboard_fn_state(void) const {
    return keyboard_fn_state_;
  }

  void set_keyboard_fn_state(bool value) {
    keyboard_fn_state_ = value;
  }

  bool get_swipe_scroll_direction(void) const {
    return swipe_scroll_direction_;
  }

  void set_swipe_scroll_direction(bool value) {
    swipe_scroll_direction_ = value;
  }

  uint8_t get_keyboard_type(void) const {
    return keyboard_type_;
  }

  void set_keyboard_type(uint8_t value) {
    keyboard_type_ = value;
  }

  bool operator==(const system_preferences& other) const {
    return keyboard_fn_state_ == other.keyboard_fn_state_ &&
           swipe_scroll_direction_ == other.swipe_scroll_direction_ &&
           keyboard_type_ == other.keyboard_type_;
  }

  bool operator!=(const system_preferences& other) const { return !(*this == other); }

private:
  bool keyboard_fn_state_;
  bool swipe_scroll_direction_;
  uint8_t keyboard_type_;
};

class types final {
public:
  // Find operation_type from operation_type_*_struct .
  static boost::optional<operation_type> find_operation_type(const std::vector<uint8_t>& buffer) {
    if (buffer.empty()) {
      return boost::none;
    }
    return operation_type(buffer[0]);
  }

  static device_id make_new_device_id(const std::shared_ptr<device_detail>& device_detail) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static auto id = device_id::zero;

    id = device_id(static_cast<uint32_t>(id) + 1);

    {
      std::lock_guard<std::mutex> lock(get_device_id_map_mutex());

      auto& map = get_device_id_map();
      map[id] = device_detail;
    }

    return id;
  }

  static void detach_device_id(device_id device_id) {
    std::lock_guard<std::mutex> lock(get_device_id_map_mutex());

    auto& map = get_device_id_map();
    map.erase(device_id);
  }

  static const std::shared_ptr<device_detail> find_device_detail(device_id device_id) {
    std::lock_guard<std::mutex> lock(get_device_id_map_mutex());

    auto& map = get_device_id_map();
    auto it = map.find(device_id);
    if (it == std::end(map)) {
      return nullptr;
    }
    return it->second;
  }

  static boost::optional<modifier_flag> make_modifier_flag(key_code key_code) {
    // make_modifier_flag(key_code::caps_lock) == boost::none

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
        return boost::none;
    }
  }

  static boost::optional<modifier_flag> make_modifier_flag(hid_usage_page usage_page, hid_usage usage) {
    if (auto key_code = make_key_code(usage_page, usage)) {
      return make_modifier_flag(*key_code);
    }
    return boost::none;
  }

  static boost::optional<modifier_flag> make_modifier_flag(const hid_value& hid_value) {
    if (auto hid_usage_page = hid_value.get_hid_usage_page()) {
      if (auto hid_usage = hid_value.get_hid_usage()) {
        return make_modifier_flag(*hid_usage_page,
                                  *hid_usage);
      }
    }
    return boost::none;
  }

  static boost::optional<pqrs::karabiner_virtual_hid_device::hid_report::modifier> make_hid_report_modifier(modifier_flag modifier_flag) {
    switch (modifier_flag) {
      case modifier_flag::left_control:
        return pqrs::karabiner_virtual_hid_device::hid_report::modifier::left_control;
      case modifier_flag::left_shift:
        return pqrs::karabiner_virtual_hid_device::hid_report::modifier::left_shift;
      case modifier_flag::left_option:
        return pqrs::karabiner_virtual_hid_device::hid_report::modifier::left_option;
      case modifier_flag::left_command:
        return pqrs::karabiner_virtual_hid_device::hid_report::modifier::left_command;
      case modifier_flag::right_control:
        return pqrs::karabiner_virtual_hid_device::hid_report::modifier::right_control;
      case modifier_flag::right_shift:
        return pqrs::karabiner_virtual_hid_device::hid_report::modifier::right_shift;
      case modifier_flag::right_option:
        return pqrs::karabiner_virtual_hid_device::hid_report::modifier::right_option;
      case modifier_flag::right_command:
        return pqrs::karabiner_virtual_hid_device::hid_report::modifier::right_command;
      default:
        return boost::none;
    }
  }

  static boost::optional<key_code> make_key_code(modifier_flag modifier_flag) {
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
  static const std::vector<std::pair<std::string, key_code>>& get_key_code_name_value_pairs(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static std::vector<std::pair<std::string, key_code>> pairs({
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

    return pairs;
  }

  static const std::unordered_map<std::string, key_code>& get_key_code_name_value_map(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static std::unordered_map<std::string, key_code> map;

    if (map.empty()) {
      for (const auto& pair : get_key_code_name_value_pairs()) {
        auto it = map.find(pair.first);
        if (it != std::end(map)) {
          logger::get_logger().error("duplicate entry in get_key_code_name_value_pairs: {0}", pair.first);
        } else {
          map.emplace(pair.first, pair.second);
        }
      }
    }

    return map;
  }

  static boost::optional<key_code> make_key_code(const std::string& name) {
    auto& map = get_key_code_name_value_map();
    auto it = map.find(name);
    if (it == map.end()) {
      logger::get_logger().error("unknown key_code: \"{0}\"", name);
      return boost::none;
    }
    return it->second;
  }

  static boost::optional<std::string> make_key_code_name(key_code key_code) {
    for (const auto& pair : get_key_code_name_value_pairs()) {
      if (pair.second == key_code) {
        return pair.first;
      }
    }
    return boost::none;
  }

  static boost::optional<key_code> make_key_code(hid_usage_page usage_page, hid_usage usage) {
    auto u = static_cast<uint32_t>(usage);

    switch (usage_page) {
      case hid_usage_page::keyboard_or_keypad:
        if (kHIDUsage_KeyboardErrorUndefined < u && u < kHIDUsage_Keyboard_Reserved) {
          return key_code(u);
        }
        break;

      case hid_usage_page::apple_vendor_top_case:
        switch (usage) {
          case hid_usage::av_top_case_keyboard_fn:
            return key_code::fn;
          case hid_usage::av_top_case_brightness_up:
            return key_code::apple_top_case_display_brightness_increment;
          case hid_usage::av_top_case_brightness_down:
            return key_code::apple_top_case_display_brightness_decrement;
          case hid_usage::av_top_case_illumination_up:
            return key_code::illumination_increment;
          case hid_usage::av_top_case_illumination_down:
            return key_code::illumination_decrement;
          default:
            break;
        }
        break;

      case hid_usage_page::apple_vendor_keyboard:
        switch (usage) {
          case hid_usage::apple_vendor_keyboard_dashboard:
            return key_code::dashboard;
          case hid_usage::apple_vendor_keyboard_function:
            return key_code::fn;
          case hid_usage::apple_vendor_keyboard_launchpad:
            return key_code::launchpad;
          case hid_usage::apple_vendor_keyboard_expose_all:
            return key_code::mission_control;
          case hid_usage::apple_vendor_keyboard_brightness_up:
            return key_code::apple_display_brightness_increment;
          case hid_usage::apple_vendor_keyboard_brightness_down:
            return key_code::apple_display_brightness_decrement;
          default:
            break;
        }
        break;

      default:
        break;
    }

    return boost::none;
  }

  static boost::optional<key_code> make_key_code(const hid_value& hid_value) {
    if (auto hid_usage_page = hid_value.get_hid_usage_page()) {
      if (auto hid_usage = hid_value.get_hid_usage()) {
        return make_key_code(*hid_usage_page,
                             *hid_usage);
      }
    }
    return boost::none;
  }

  static boost::optional<hid_usage_page> make_hid_usage_page(key_code key_code) {
    switch (key_code) {
      case key_code::fn:
      case key_code::illumination_decrement:
      case key_code::illumination_increment:
      case key_code::apple_top_case_display_brightness_decrement:
      case key_code::apple_top_case_display_brightness_increment:
        return hid_usage_page::apple_vendor_top_case;

      case key_code::dashboard:
      case key_code::launchpad:
      case key_code::mission_control:
      case key_code::apple_display_brightness_decrement:
      case key_code::apple_display_brightness_increment:
        return hid_usage_page::apple_vendor_keyboard;

      case key_code::mute:
      case key_code::volume_decrement:
      case key_code::volume_increment:
      case key_code::display_brightness_decrement:
      case key_code::display_brightness_increment:
      case key_code::rewind:
      case key_code::play_or_pause:
      case key_code::fastforward:
      case key_code::eject:
        return hid_usage_page::consumer;

      case key_code::vk_none:
        return boost::none;

      default:
        return hid_usage_page::keyboard_or_keypad;
    }
  }

  static boost::optional<hid_usage> make_hid_usage(key_code key_code) {
    switch (key_code) {
      case key_code::fn:
        return hid_usage::av_top_case_keyboard_fn;

      case key_code::illumination_decrement:
        return hid_usage::av_top_case_illumination_down;

      case key_code::illumination_increment:
        return hid_usage::av_top_case_illumination_up;

      case key_code::apple_top_case_display_brightness_decrement:
        return hid_usage::av_top_case_brightness_down;

      case key_code::apple_top_case_display_brightness_increment:
        return hid_usage::av_top_case_brightness_up;

      case key_code::dashboard:
        return hid_usage::apple_vendor_keyboard_dashboard;

      case key_code::launchpad:
        return hid_usage::apple_vendor_keyboard_launchpad;

      case key_code::mission_control:
        return hid_usage::apple_vendor_keyboard_expose_all;

      case key_code::apple_display_brightness_decrement:
        return hid_usage::apple_vendor_keyboard_brightness_down;

      case key_code::apple_display_brightness_increment:
        return hid_usage::apple_vendor_keyboard_brightness_up;

      case key_code::mute:
        return hid_usage::csmr_mute;

      case key_code::volume_decrement:
        return hid_usage::csmr_volume_decrement;

      case key_code::volume_increment:
        return hid_usage::csmr_volume_increment;

      case key_code::display_brightness_decrement:
        return hid_usage::csmr_display_brightness_decrement;

      case key_code::display_brightness_increment:
        return hid_usage::csmr_display_brightness_increment;

      case key_code::rewind:
        return hid_usage::csmr_rewind;

      case key_code::play_or_pause:
        return hid_usage::csmr_play_or_pause;

      case key_code::fastforward:
        return hid_usage::csmr_fastforward;

      case key_code::eject:
        return hid_usage::csmr_eject;

      case key_code::vk_none:
        return boost::none;

      default:
        return hid_usage(key_code);
    }
  }

  static const std::vector<std::pair<std::string, consumer_key_code>>& get_consumer_key_code_name_value_pairs(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static std::vector<std::pair<std::string, consumer_key_code>> pairs({
        {"power", consumer_key_code::power},
        {"display_brightness_increment", consumer_key_code::display_brightness_increment},
        {"display_brightness_decrement", consumer_key_code::display_brightness_decrement},
        {"fastforward", consumer_key_code::fastforward},
        {"rewind", consumer_key_code::rewind},
        {"scan_next_track", consumer_key_code::scan_next_track},
        {"scan_previous_track", consumer_key_code::scan_previous_track},
        {"eject", consumer_key_code::eject},
        {"play_or_pause", consumer_key_code::play_or_pause},
        {"mute", consumer_key_code::mute},
        {"volume_increment", consumer_key_code::volume_increment},
        {"volume_decrement", consumer_key_code::volume_decrement},
    });

    return pairs;
  }

  static const std::unordered_map<std::string, consumer_key_code>& get_consumer_key_code_name_value_map(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static std::unordered_map<std::string, consumer_key_code> map;

    if (map.empty()) {
      for (const auto& pair : get_consumer_key_code_name_value_pairs()) {
        auto it = map.find(pair.first);
        if (it != std::end(map)) {
          logger::get_logger().error("duplicate entry in get_consumer_key_code_name_value_pairs: {0}", pair.first);
        } else {
          map.emplace(pair.first, pair.second);
        }
      }
    }

    return map;
  }

  static boost::optional<consumer_key_code> make_consumer_key_code(const std::string& name) {
    auto& map = get_consumer_key_code_name_value_map();
    auto it = map.find(name);
    if (it == map.end()) {
      logger::get_logger().error("unknown consumer_key_code: \"{0}\"", name);
      return boost::none;
    }
    return it->second;
  }

  static boost::optional<std::string> make_consumer_key_code_name(consumer_key_code consumer_key_code) {
    for (const auto& pair : get_consumer_key_code_name_value_pairs()) {
      if (pair.second == consumer_key_code) {
        return pair.first;
      }
    }
    return boost::none;
  }

  static boost::optional<consumer_key_code> make_consumer_key_code(hid_usage_page usage_page, hid_usage usage) {
    auto u = static_cast<uint32_t>(usage);

    switch (usage_page) {
      case hid_usage_page::consumer:
        switch (consumer_key_code(u)) {
          case consumer_key_code::power:
          case consumer_key_code::display_brightness_increment:
          case consumer_key_code::display_brightness_decrement:
          case consumer_key_code::fastforward:
          case consumer_key_code::rewind:
          case consumer_key_code::scan_next_track:
          case consumer_key_code::scan_previous_track:
          case consumer_key_code::eject:
          case consumer_key_code::play_or_pause:
          case consumer_key_code::mute:
          case consumer_key_code::volume_increment:
          case consumer_key_code::volume_decrement:
            return consumer_key_code(u);
        }

      default:
        break;
    }

    return boost::none;
  }

  static boost::optional<consumer_key_code> make_consumer_key_code(const hid_value& hid_value) {
    if (auto hid_usage_page = hid_value.get_hid_usage_page()) {
      if (auto hid_usage = hid_value.get_hid_usage()) {
        return make_consumer_key_code(*hid_usage_page,
                                      *hid_usage);
      }
    }
    return boost::none;
  }

  static boost::optional<hid_usage_page> make_hid_usage_page(consumer_key_code consumer_key_code) {
    return hid_usage_page::consumer;
  }

  static boost::optional<hid_usage> make_hid_usage(consumer_key_code consumer_key_code) {
    return hid_usage(static_cast<uint32_t>(consumer_key_code));
  }

  static const std::vector<std::pair<std::string, pointing_button>>& get_pointing_button_name_value_pairs(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static std::vector<std::pair<std::string, pointing_button>> pairs({
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

    return pairs;
  }

  static const std::unordered_map<std::string, pointing_button>& get_pointing_button_name_value_map(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static std::unordered_map<std::string, pointing_button> map;

    if (map.empty()) {
      for (const auto& pair : get_pointing_button_name_value_pairs()) {
        auto it = map.find(pair.first);
        if (it != std::end(map)) {
          logger::get_logger().error("duplicate entry in get_pointing_button_name_value_pairs: {0}", pair.first);
        } else {
          map.emplace(pair.first, pair.second);
        }
      }
    }

    return map;
  }

  static boost::optional<pointing_button> make_pointing_button(const std::string& name) {
    auto& map = get_pointing_button_name_value_map();
    auto it = map.find(name);
    if (it == map.end()) {
      logger::get_logger().error("unknown pointing_button: \"{0}\"", name);
      return boost::none;
    }
    return it->second;
  }

  static boost::optional<std::string> make_pointing_button_name(pointing_button pointing_button) {
    for (const auto& pair : get_pointing_button_name_value_pairs()) {
      if (pair.second == pointing_button) {
        return pair.first;
      }
    }
    return boost::none;
  }

  static boost::optional<pointing_button> make_pointing_button(hid_usage_page usage_page, hid_usage usage) {
    if (usage_page == hid_usage_page::button) {
      return pointing_button(usage);
    }
    return boost::none;
  }

  static boost::optional<pointing_button> make_pointing_button(const hid_value& hid_value) {
    if (auto hid_usage_page = hid_value.get_hid_usage_page()) {
      if (auto hid_usage = hid_value.get_hid_usage()) {
        return make_pointing_button(*hid_usage_page,
                                    *hid_usage);
      }
    }
    return boost::none;
  }

private:
  static std::unordered_map<device_id, std::shared_ptr<device_detail>>& get_device_id_map(void) {
    static std::unordered_map<device_id, std::shared_ptr<device_detail>> map;
    return map;
  }

  static std::mutex& get_device_id_map_mutex(void) {
    static std::mutex mutex;
    return mutex;
  }
};

struct operation_type_grabbable_state_changed_struct {
  operation_type_grabbable_state_changed_struct(void) : operation_type(operation_type::grabbable_state_changed) {}

  const operation_type operation_type;
  grabbable_state grabbable_state;
};

struct operation_type_connect_console_user_server_struct {
  operation_type_connect_console_user_server_struct(void) : operation_type(operation_type::connect_console_user_server) {
    strlcpy(user_core_configuration_file_path,
            constants::get_user_core_configuration_file_path().c_str(),
            sizeof(user_core_configuration_file_path));
  }

  const operation_type operation_type;
  pid_t pid;
  char user_core_configuration_file_path[_POSIX_PATH_MAX];
};

struct operation_type_system_preferences_updated_struct {
  operation_type_system_preferences_updated_struct(void) : operation_type(operation_type::system_preferences_updated) {}

  const operation_type operation_type;
  system_preferences system_preferences;
};

struct operation_type_frontmost_application_changed_struct {
  operation_type_frontmost_application_changed_struct(void) : operation_type(operation_type::frontmost_application_changed) {
    bundle_identifier[0] = '\0';
    file_path[0] = '\0';
  }

  const operation_type operation_type;
  char bundle_identifier[256];
  char file_path[_POSIX_PATH_MAX];
};

struct operation_type_input_source_changed_struct {
  operation_type_input_source_changed_struct(void) : operation_type(operation_type::input_source_changed) {
    language[0] = '\0';
    input_source_id[0] = '\0';
    input_mode_id[0] = '\0';
  }

  const operation_type operation_type;
  char language[256];
  char input_source_id[256];
  char input_mode_id[256];
};

struct operation_type_shell_command_execution_struct {
  operation_type_shell_command_execution_struct(void) : operation_type(operation_type::shell_command_execution) {
    shell_command[0] = '\0';
  }

  const operation_type operation_type;
  char shell_command[256];
};

struct operation_type_select_input_source_struct {
  operation_type_select_input_source_struct(void) : operation_type(operation_type::select_input_source) {
    language[0] = '\0';
    input_source_id[0] = '\0';
    input_mode_id[0] = '\0';
  }

  const operation_type operation_type;
  absolute_time time_stamp;
  char language[256];
  char input_source_id[256];
  char input_mode_id[256];
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

KRBN_TYPES_STREAM_OUTPUT(operation_type);
KRBN_TYPES_STREAM_OUTPUT(device_id);

#undef KRBN_TYPES_STREAM_OUTPUT
} // namespace krbn
