#pragma once

// `krbn::keyboard_fallback_loop_guard` can be used safely in a single-threaded environment.

#include "logger.hpp"
#include "types.hpp"
#include <chrono>
#include <deque>
#include <optional>
#include <unordered_map>

namespace krbn {
class keyboard_fallback_loop_guard final {
public:
  keyboard_fallback_loop_guard(std::chrono::milliseconds rate_window = std::chrono::milliseconds(100),
                               size_t rate_threshold = 10,
                               std::chrono::milliseconds suspension_duration = std::chrono::milliseconds(1000))
      : rate_window_(rate_window),
        rate_threshold_(rate_threshold),
        suspension_duration_(suspension_duration) {
  }

  bool should_suspend(const event_queue::event& event,
                      const std::chrono::steady_clock::time_point& now) {
    if (suspended_until_) {
      if (now < *suspended_until_) {
        return true;
      }

      logger::get_logger()->info("event_tap_monitor keyboard fallback resumed");
      suspended_until_ = std::nullopt;
      timestamps_.clear();
    }

    auto momentary_switch_event = event.get_if<krbn::momentary_switch_event>();
    if (!momentary_switch_event ||
        !momentary_switch_event->valid() ||
        momentary_switch_event->pointing_button()) {
      return false;
    }

    auto& timestamps = timestamps_[*momentary_switch_event];
    purge_expired_timestamps(timestamps, now);
    timestamps.push_back(now);

    if (timestamps.size() <= rate_threshold_) {
      return false;
    }

    suspended_until_ = now + suspension_duration_;
    timestamps_.clear();

    logger::get_logger()->error("event_tap_monitor keyboard fallback suspended for {0}ms",
                                std::chrono::duration_cast<std::chrono::milliseconds>(suspension_duration_).count());

    return true;
  }

private:
  using timestamps = std::deque<std::chrono::steady_clock::time_point>;

  void purge_expired_timestamps(timestamps& timestamps,
                                const std::chrono::steady_clock::time_point& now) const {
    while (!timestamps.empty() &&
           timestamps.front() <= now - rate_window_) {
      timestamps.pop_front();
    }
  }

  std::chrono::milliseconds rate_window_;
  size_t rate_threshold_;
  std::chrono::milliseconds suspension_duration_;
  std::unordered_map<momentary_switch_event, timestamps> timestamps_;
  std::optional<std::chrono::steady_clock::time_point> suspended_until_;
};
} // namespace krbn
