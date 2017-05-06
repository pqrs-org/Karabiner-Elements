#pragma once

#include "boost_defs.hpp"

#include "event_queue.hpp"
#include "types.hpp"
#include <boost/optional.hpp>
#include <vector>

namespace krbn {
class pressed_physical_keys_counter final {
public:
  bool empty(device_id device_id) {
    for (const auto& pair : pressed_keys_) {
      if (pair.first == device_id) {
        return false;
      }
    }
    return true;
  }

  bool is_pointing_button_pressed(device_id device_id) {
    for (const auto& pair : pressed_keys_) {
      if (pair.first == device_id &&
          pair.second.get_type() == event_queue::queued_event::event::type::pointing_button) {
        return true;
      }
    }
    return false;
  }

  bool update(const event_queue::queued_event& queued_event) {
    auto key_code = queued_event.get_event().get_key_code();
    auto pointing_button = queued_event.get_event().get_pointing_button();

    if (key_code || pointing_button) {
      auto device_id = queued_event.get_device_id();
      auto event = queued_event.get_event();

      if (queued_event.get_event_type() == event_type::key_down) {
        emplace_back_event(device_id, event);
      } else {
        erase_all_matched_events(device_id, event);
      }

      return true;
    }

    return false;
  }

  void emplace_back_event(device_id device_id,
                          const event_queue::queued_event::event& event) {
    pressed_keys_.emplace_back(device_id, event);
  }

  void erase_all_matched_events(device_id device_id) {
    pressed_keys_.erase(std::remove_if(std::begin(pressed_keys_),
                                       std::end(pressed_keys_),
                                       [&](const auto& pair) {
                                         return pair.first == device_id;
                                       }),
                        std::end(pressed_keys_));
  }

  void erase_all_matched_events(device_id device_id,
                                const event_queue::queued_event::event& event) {
    pressed_keys_.erase(std::remove_if(std::begin(pressed_keys_),
                                       std::end(pressed_keys_),
                                       [&](const auto& pair) {
                                         // key_code or pointing_button
                                         return pair.first == device_id &&
                                                pair.second == event;
                                       }),
                        std::end(pressed_keys_));
  }

private:
  std::vector<std::pair<device_id, event_queue::queued_event::event>> pressed_keys_;
};
} // namespace krbn
