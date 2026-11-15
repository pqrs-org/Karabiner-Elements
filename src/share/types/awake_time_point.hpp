#pragma once

#include <chrono>
#include <time.h>

namespace krbn {
class awake_clock final {
public:
  using duration = std::chrono::milliseconds;
  using rep = duration::rep;
  using period = duration::period;
  using time_point = std::chrono::time_point<awake_clock>;

  static constexpr bool is_steady = true;

  static time_point now() noexcept {
    // CLOCK_UPTIME_RAW is monotonic and does not advance while the system is asleep.
    timespec time{};
    clock_gettime(CLOCK_UPTIME_RAW, &time);

    return time_point(
        std::chrono::duration_cast<duration>(
            std::chrono::seconds(time.tv_sec) +
            std::chrono::nanoseconds(time.tv_nsec)));
  }
};

using awake_time_point = awake_clock::time_point;
} // namespace krbn
