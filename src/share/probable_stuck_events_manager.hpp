#pragma once

// `krbn::probable_stuck_events_manager` can be used safely in a multi-threaded environment.

#include "types.hpp"
#include <set>

namespace krbn {
// `probable_stuck_events_manager` manages events which may be stuck with ungrabbed device.
//
// On macOS, even after karabiner_grabber has seized a device, the original hardware key press status is retained.
// Once the device is seized, karabiner_grabber emits events from a virtual HID keyboard,
// so the status of the hardware keys does not change after being seized
//
// This behavior leads to several problems:
//   - If a key, such as an arrow key, is held down before the device is seized, the key repeat does not stop.
//   - If a modifier key is pressed before the device is seized, that modifier is always applied to mouse button events.
//
// Therefore, if it is detected that a key has been continuously pressed since before the device was seized,
// it is necessary to temporarily un-seize the device.
//
// Specifically, for each key, if a key_up event is received before a key_down event,
// it is determined that the key was pressed before the device was seized.

class probable_stuck_events_manager {
public:
  probable_stuck_events_manager(void) {
  }

  bool update(const momentary_switch_event& event,
              event_type t,
              device_state state) {
    if (!event.valid()) {
      return false;
    }

    auto previous_probable_stuck_event = find_probable_stuck_event();

    {
      std::lock_guard<std::mutex> lock(mutex_);

      //
      // Handle event
      //

      auto previous_probable_stuck_events = probable_stuck_events_;

      switch (t) {
        case event_type::key_down:
          key_down_arrived_events_.insert(event);
          key_up_events_.erase(event);
          exceptional_key_up_events_.erase(event);

          // When the device is not seized, more aggressive actions can be taken at key_down event.
          // If the pressed key is neither a modifier nor a mouse button, key repeat will occur with that key,
          // allowing other keys to be removed from probable_stuck_events_.

          if (state == device_state::ungrabbed) {
            if (event.interrupts_key_repeat()) {
              erase_except_modifier_flags(event.get_usage_pair().get_usage_page());
            }

            probable_stuck_events_.insert(event);
          }

          break;

        case event_type::key_up: {
          // Some devices send key_up event periodically without paired `key_down`.
          // For example, Swiftpoint ProPoint sends pqrs::hid::usage::consumer::play_or_pause key_up after each button1 click.
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
    }

    return find_probable_stuck_event() != previous_probable_stuck_event;
  }

  void clear(void) {
    std::lock_guard<std::mutex> lock(mutex_);

    probable_stuck_events_.clear();
    key_down_arrived_events_.clear();
    key_up_events_.clear();
    // Do not clear `exceptional_key_up_events_` here.
  }

  std::optional<momentary_switch_event> find_probable_stuck_event(void) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!probable_stuck_events_.empty()) {
      return *(std::begin(probable_stuck_events_));
    }

    return std::nullopt;
  }

private:
  void erase_except_modifier_flags(pqrs::hid::usage_page::value_t usage_page) {
    auto it = std::begin(probable_stuck_events_);
    while (it != std::end(probable_stuck_events_)) {
      if (it->get_usage_pair().get_usage_page() == usage_page && !it->modifier_flag()) {
        it = probable_stuck_events_.erase(it);
        continue;
      }
      std::advance(it, 1);
    }
  }

  std::set<momentary_switch_event> probable_stuck_events_;
  std::set<momentary_switch_event> key_down_arrived_events_;
  std::set<momentary_switch_event> key_up_events_;
  // Some devices sends key_up events continuously and never sends paired key_down events.
  // Store them into `exceptional_key_up_events_`.
  std::set<momentary_switch_event> exceptional_key_up_events_;
  mutable std::mutex mutex_;
};
} // namespace krbn
