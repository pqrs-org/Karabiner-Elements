#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include "absolute_time_point.hpp"
#include "impl/chrono.hpp"
#include <chrono>
#include <mach/mach_time.h>
#include <time.h>

namespace pqrs {
namespace osx {
namespace chrono {
inline absolute_time_point mach_absolute_time_point(void) {
  return absolute_time_point(mach_absolute_time());
}

inline std::chrono::nanoseconds make_nanoseconds(absolute_time_duration time) {
  auto& t = impl::get_mach_timebase_info_data();
  if (t.numer != t.denom && t.denom != 0) {
    return std::chrono::nanoseconds(type_safe::get(time) * t.numer / t.denom);
  }
  return std::chrono::nanoseconds(type_safe::get(time));
}

inline std::chrono::milliseconds make_milliseconds(absolute_time_duration time) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(make_nanoseconds(time));
}

inline absolute_time_duration make_absolute_time_duration(std::chrono::nanoseconds time) {
  auto& t = impl::get_mach_timebase_info_data();
  if (t.numer != t.denom && t.numer != 0) {
    return absolute_time_duration(time.count() * t.denom / t.numer);
  }
  return absolute_time_duration(time.count());
}
} // namespace chrono
} // namespace osx
} // namespace pqrs
