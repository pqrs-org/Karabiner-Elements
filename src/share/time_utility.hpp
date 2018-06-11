#pragma once

#include "logger.hpp"
#include <mach/mach_time.h>
#include <time.h>

namespace krbn {
class time_utility final {
public:
  static uint64_t absolute_to_nano(uint64_t absolute_time) {
    auto& t = get_mach_timebase_info_data();
    if (t.numer != t.denom && t.denom != 0) {
      return absolute_time * t.numer / t.denom;
    }
    return absolute_time;
  }

  static uint64_t nano_to_absolute(uint64_t absolute_time) {
    auto& t = get_mach_timebase_info_data();
    if (t.numer != t.denom && t.numer != 0) {
      return absolute_time * t.denom / t.numer;
    }
    return absolute_time;
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
