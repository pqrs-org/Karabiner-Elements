#pragma once

// `krbn::orphan_key_up_events_manager` can be used safely in a multi-threaded environment.

#include "types.hpp"
#include <set>

namespace krbn {
class orphan_key_up_events_manager {
public:
  orphan_key_up_events_manager(void) : last_time_stamp_(0) {
  }

  bool update(const key_down_up_valued_event& event,
              event_type t,
              absolute_time_point time_stamp,
              device_state state) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Skip old event.
    if (time_stamp < last_time_stamp_) {
      return false;
    }

    last_time_stamp_ = time_stamp;

    auto previous_size = orphan_key_up_events_.size();

    switch (t) {
      case event_type::key_down:
        key_down_arrived_events_.insert(event);
        break;

      case event_type::key_up: {
        // orphan_key_up_events_manager is used to detect keys which are pressed before grabbed.
        //
        // For example, we have to ungrab a device at (4) to be able to stop key repeat.
        //
        // 0. A keyboard is not grabbed yet.
        // 1. `key_code::a` is pressed.
        // 2. `a` is repeated by macOS.
        // 3. The keyboard is grabbed.
        // 4. `key_code::a` is released.
        // 5. `a` is still repeated by macOS because `key_up` is send from another device (virtual keyboard).
        //
        // In this typical case, we have to remove orphan key at `key_up`.
        // If orphan key is removed at `key_down`,
        // the keyboard will be re-grabbed when key is still pressed.
        // It causes the same key repeat issue again.

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

    return orphan_key_up_events_.size() != previous_size;
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
