#pragma once

// `krbn::probable_stuck_events_manager` can be used safely in a multi-threaded environment.

#include "types.hpp"
#include <set>

namespace krbn {
// `probable_stuck_events_manager` manages events which may be stuck with ungrabbed device.
//
// macOS keeps the key state after the device is grabbed.
// This behavior causes several problems:
//   - A key repeating cannot be stopped if the key is held down before the device is grabbed.
//   - Modifier keys will be stuck if they are held down before the device is grabbed.
//
// Thus, we should not grab (or should ungrab) the device in such cases.
// `probable_stuck_events_manager` is used to detect such states.

class probable_stuck_events_manager {
public:
  probable_stuck_events_manager(void) : last_time_stamp_(0) {
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

    auto previous_size = probable_stuck_events_.size();

    switch (t) {
      case event_type::key_down:
        key_down_arrived_events_.insert(event);

        // We register a key (or button) to probable_stuck_events_
        // unless the event is sent from grabbed device.
        // (The key may be repeating.)

        if (state == device_state::ungrabbed) {
          if (event.find<key_code>()) {
            if (event.modifier_flag()) {
              // Do nothing (Do not erase existing keys.)

            } else {
              // Erase normal keys from probable_stuck_events_
              // because the new key cancels existing keys repeat.

              erase_except_modifier_flags<key_code>();
            }
          } else if (event.find<consumer_key_code>()) {
            // Erase other keys. (same as `key_code`.)

            erase_except_modifier_flags<consumer_key_code>();
          }
          // Do nothing with pointing_button.

          probable_stuck_events_.insert(event);
        }

        break;

      case event_type::key_up: {
        // The key was held down before the device is grabbed
        // if `key_up` is came without paired `key_down`.

        if (key_down_arrived_events_.find(event) == std::end(key_down_arrived_events_)) {
          probable_stuck_events_.insert(event);
        } else {
          probable_stuck_events_.erase(event);
        }
        break;
      }

      case event_type::single:
        // Do nothing
        break;
    }

    return probable_stuck_events_.size() != previous_size;
  }

  void clear(void) {
    std::lock_guard<std::mutex> lock(mutex_);

    probable_stuck_events_.clear();
    key_down_arrived_events_.clear();
    last_time_stamp_ = absolute_time_point(0);
  }

  std::optional<key_down_up_valued_event> find_probable_stuck_event(void) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!probable_stuck_events_.empty()) {
      return *(std::begin(probable_stuck_events_));
    }

    return std::nullopt;
  }

private:
  template <typename T>
  void erase_except_modifier_flags(void) {
    auto it = std::begin(probable_stuck_events_);
    while (it != std::end(probable_stuck_events_)) {
      if (it->find<T>() && !it->modifier_flag()) {
        it = probable_stuck_events_.erase(it);
      } else {
        std::advance(it, 1);
      }
    }
  }

  std::set<key_down_up_valued_event> probable_stuck_events_;
  std::set<key_down_up_valued_event> key_down_arrived_events_;
  absolute_time_point last_time_stamp_;
  mutable std::mutex mutex_;
};
} // namespace krbn
