#pragma once

#include <IOKit/IOKitLib.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <cstdint>
#include <memory>
#include <vector>

class manipulator {
public:
  enum class key_code : uint32_t {
    // 0x00 - 0xff is usage page

    extra_ = 0x1000,
    vk_none,
    // keys will be handled in console_user_client
    vk_fn_modifier,
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

  enum class manipulator_type : uint32_t {
    keytokey,
  };
  enum class autogen_id : uint64_t {
    max_ = UINT64_MAX,
  };
  enum class add_data_type : uint32_t {
    key_code,
    modifier_flag,
  };
  enum class add_value : uint32_t {};

#include "manipulator/modifier_flag_manager.hpp"
};
