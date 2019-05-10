#pragma once

// `krbn::orphan_key_up_events_manager` can be used safely in a multi-threaded environment.

#include "types.hpp"
#include <set>

namespace krbn {
class orphan_key_up_events_manager {
public:
  orphan_key_up_events_manager(void) : last_time_stamp_(0) {
  }

  void update(const key_down_up_valued_event& event,
              event_type t,
              absolute_time_point time_stamp) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Skip old event.
    if (time_stamp < last_time_stamp_) {
      return;
    }

    last_time_stamp_ = time_stamp;

    switch (t) {
      case event_type::key_down:
        key_down_arrived_events_.insert(event);
        break;

      case event_type::key_up: {
        if (key_down_arrived_events_.find(event) == std::end(key_down_arrived_events_)) {
          orphan_key_up_events_.insert(event);
        } else {
          orphan_key_up_events_.erase(event);
        }
        break;
      }

      case event_type::single:
        // Do nothing
        break;
    }
  }

  void clear(void) {
    std::lock_guard<std::mutex> lock(mutex_);

    orphan_key_up_events_.clear();
    key_down_arrived_events_.clear();
    last_time_stamp_ = absolute_time_point(0);
  }

  std::optional<key_down_up_valued_event> find_orphan_key_up_event(void) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!orphan_key_up_events_.empty()) {
      return *(std::begin(orphan_key_up_events_));
    }

    return std::nullopt;
  }

private:
  std::set<key_down_up_valued_event> orphan_key_up_events_;
  std::set<key_down_up_valued_event> key_down_arrived_events_;
  absolute_time_point last_time_stamp_;
  mutable std::mutex mutex_;
};
} // namespace krbn
