#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <mach/mach_time.h>
#include <time.h>

namespace pqrs::osx::chrono::impl {
[[nodiscard]] inline const mach_timebase_info_data_t& get_mach_timebase_info_data() noexcept {
  static const auto mach_timebase_info_data = [] {
    mach_timebase_info_data_t data{};
    mach_timebase_info(&data);
    return data;
  }();
  return mach_timebase_info_data;
}
} // namespace pqrs::osx::chrono::impl
