#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include <IOKit/hidsystem/IOHIDShared.h>

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

class types {
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
