#pragma once

#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <cstdint>
#include <memory>
#include <vector>

class manipulator final {
public:
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
