#pragma once

#include "logger.hpp"
#include "types.hpp"
#include <chrono>
#include <mach/mach_time.h>
#include <time.h>

namespace krbn {
class time_utility final {
public:
  static absolute_time mach_absolute_time(void) {
    return absolute_time(::mach_absolute_time());
  }

  static std::chrono::nanoseconds absolute_to_nano(absolute_time time) {
    auto& t = get_mach_timebase_info_data();
    if (t.numer != t.denom && t.denom != 0) {
      return std::chrono::nanoseconds(type_safe::get(time) * t.numer / t.denom);
    }
    return std::chrono::nanoseconds(type_safe::get(time));
  }

  static std::chrono::milliseconds absolute_to_milliseconds(absolute_time time) {
    auto ns = std::chrono::nanoseconds(absolute_to_nano(time));
    return std::chrono::duration_cast<std::chrono::milliseconds>(ns);
  }

  static absolute_time nano_to_absolute(std::chrono::nanoseconds time) {
    auto& t = get_mach_timebase_info_data();
    if (t.numer != t.denom && t.numer != 0) {
      return absolute_time(time.count() * t.denom / t.numer);
    }
    return absolute_time(time.count());
  }

  static std::string make_current_local_yyyymmdd_string(void) {
    auto t = time(nullptr);

    tm tm;
    localtime_r(&t, &tm);

    return fmt::format("{0:04d}{1:02d}{2:02d}",
                       tm.tm_year + 1900,
                       tm.tm_mon + 1,
                       tm.tm_mday);
  }

private:
  static const mach_timebase_info_data_t& get_mach_timebase_info_data(void) {
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
};
} // namespace krbn
