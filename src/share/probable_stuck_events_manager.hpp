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

  bool update(const momentary_switch_event& event,
              event_type t,
              absolute_time_point time_stamp,
              device_state state) {
    std::lock_guard<std::mutex> lock(mutex_);

    //
    // Skip old event
    //

    if (time_stamp < last_time_stamp_) {
      return false;
    }

    last_time_stamp_ = time_stamp;

    //
    // Handle event
    //

    auto previous_size = probable_stuck_events_.size();

    switch (t) {
      case event_type::key_down:
        key_down_arrived_events_.insert(event);
        key_up_events_.erase(event);
        exceptional_key_up_events_.erase(event);

        // We register a key (or button) to probable_stuck_events_
        // unless the event is sent from grabbed device.
        // (The key may be repeating.)

        if (state == device_state::ungrabbed) {
          if (event.find<key_code::value_t>()) {
            if (event.modifier_flag()) {
              // Do nothing (Do not erase existing keys.)

            } else {
              // Erase normal keys from probable_stuck_events_
              // because the new key cancels existing keys repeat.

              erase_except_modifier_flags<key_code::value_t>();
            }
          } else if (event.find<consumer_key_code::value_t>()) {
            // Erase other keys. (same as `key_code`.)

            erase_except_modifier_flags<consumer_key_code::value_t>();
          }
          // Do nothing with pointing_button.

          probable_stuck_events_.insert(event);
        }

        break;

      case event_type::key_up: {
        // Some devices send key_up event periodically without paired `key_down`.
        // For example, Swiftpoint ProPoint sends consumer_key_code::play_or_pause key_up after each button1 click.
        // So, we ignore such key_up event if key_up event sent twice without key_down event.

        auto already_key_up = (key_up_events_.find(event) != std::end(key_up_events_));

        if (already_key_up) {
          exceptional_key_up_events_.insert(event);
          probable_stuck_events_.erase(event);
        } else {
          // The key was held down before the device is grabbed
          // if `key_up` is came without paired `key_down`.

          auto key_down_arrived = (key_down_arrived_events_.find(event) != std::end(key_down_arrived_events_));

          auto exceptional = exceptional_key_up_events_.find(event) != std::end(exceptional_key_up_events_);

          if (!key_down_arrived && !exceptional) {
            probable_stuck_events_.insert(event);
          } else {
            probable_stuck_events_.erase(event);
          }
        }

        key_up_events_.insert(event);

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
    key_up_events_.clear();
    // Do not clear `exceptional_key_up_events_` here.
    last_time_stamp_ = absolute_time_point(0);
  }

  std::optional<momentary_switch_event> find_probable_stuck_event(void) const {
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

  std::set<momentary_switch_event> probable_stuck_events_;
  std::set<momentary_switch_event> key_down_arrived_events_;
  std::set<momentary_switch_event> key_up_events_;
  // Some devices sends key_up events continuously and never sends paired key_down events.
  // Store them into `exceptional_key_up_events_`.
  std::set<momentary_switch_event> exceptional_key_up_events_;
  absolute_time_point last_time_stamp_;
  mutable std::mutex mutex_;
};
} // namespace krbn
