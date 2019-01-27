#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <mach/mach_time.h>
#include <mutex>
#include <time.h>

namespace pqrs {
namespace osx {
namespace chrono {
namespace impl {
inline const mach_timebase_info_data_t& get_mach_timebase_info_data(void) {
  static std::mutex mutex;
  std::lock_guard<std::mutex> guard(mutex);

  static bool once = false;
  static mach_timebase_info_data_t mach_timebase_info_data;

  if (!once) {
    once = true;
    mach_timebase_info(&mach_timebase_info_data);
  }

  return mach_timebase_info_data;
}
} // namespace impl
} // namespace chrono
} // namespace osx
} // namespace pqrs
