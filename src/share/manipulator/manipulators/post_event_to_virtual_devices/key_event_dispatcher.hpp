#pragma once

#include "queue.hpp"

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace post_event_to_virtual_devices {
class key_event_dispatcher final {
public:
  void dispatch_key_down_event(device_id device_id,
                               pqrs::hid::usage_page::value_t usage_page,
                               pqrs::hid::usage::value_t usage,
                               queue& queue,
                               absolute_time_point time_stamp) {
    // Enqueue key_down event if it is not sent yet.

    if (!key_event_exists(usage_page, usage)) {
      pressed_keys_.emplace_back(device_id, std::make_pair(usage_page, usage));
      enqueue_key_event(usage_page, usage, event_type::key_down, queue, time_stamp);
    }
  }

  void dispatch_key_up_event(pqrs::hid::usage_page::value_t usage_page,
                             pqrs::hid::usage::value_t usage,
                             queue& queue,
                             absolute_time_point time_stamp) {
    // Enqueue key_up event if it is already sent.

    if (key_event_exists(usage_page, usage)) {
      pressed_keys_.erase(std::remove_if(std::begin(pressed_keys_),
                                         std::end(pressed_keys_),
                                         [&](auto& k) {
                                           return k.second.first == usage_page &&
                                                  k.second.second == usage;
                                         }),
                          std::end(pressed_keys_));
      enqueue_key_event(usage_page, usage, event_type::key_up, queue, time_stamp);
    }
  }

  void dispatch_modifier_key_event(const modifier_flag_manager& modifier_flag_manager,
                                   queue& queue,
                                   absolute_time_point time_stamp) {
    auto modifier_flags = {
        // General modifier keys
        modifier_flag::left_control,
        modifier_flag::left_shift,
        modifier_flag::left_option,
        modifier_flag::left_command,
        modifier_flag::right_control,
        modifier_flag::right_shift,
        modifier_flag::right_option,
        modifier_flag::right_command,
        modifier_flag::fn,
        // Lock keys
        modifier_flag::caps_lock,
    };
    for (const auto& m : modifier_flags) {
      bool pressed = pressed_modifier_flags_.find(m) != std::end(pressed_modifier_flags_);

      if (modifier_flag_manager.is_pressed(m)) {
        if (!pressed) {
          momentary_switch_event e(m);
          if (e.valid()) {
            enqueue_key_event(e.get_usage_pair().get_usage_page(),
                              e.get_usage_pair().get_usage(),
                              event_type::key_down,
                              queue,
                              time_stamp);

            if (m == modifier_flag::caps_lock) {
              enqueue_key_event(e.get_usage_pair().get_usage_page(),
                                e.get_usage_pair().get_usage(),
                                event_type::key_up,
                                queue,
                                time_stamp);
            }
          }
          pressed_modifier_flags_.insert(m);
        }

      } else {
        if (pressed) {
          momentary_switch_event e(m);
          if (e.valid()) {
            if (m == modifier_flag::caps_lock) {
              enqueue_key_event(e.get_usage_pair().get_usage_page(),
                                e.get_usage_pair().get_usage(),
                                event_type::key_down,
                                queue,
                                time_stamp);
            }

            enqueue_key_event(e.get_usage_pair().get_usage_page(),
                              e.get_usage_pair().get_usage(),
                              event_type::key_up,
                              queue,
                              time_stamp);
          }
          pressed_modifier_flags_.erase(m);
        }
      }
    }
  }

  void dispatch_key_up_events_by_device_id(device_id device_id,
                                           queue& queue,
                                           absolute_time_point time_stamp) {
    while (true) {
      bool found = false;
      for (const auto& k : pressed_keys_) {
        if (k.first == device_id) {
          found = true;
          dispatch_key_up_event(k.second.first,
                                k.second.second,
                                queue,
                                time_stamp);
          break;
        }
      }
      if (!found) {
        break;
      }
    }
  }

  const std::vector<std::pair<device_id, std::pair<pqrs::hid::usage_page::value_t, pqrs::hid::usage::value_t>>>& get_pressed_keys(void) const {
    return pressed_keys_;
  }

  void insert_pressed_modifier_flag(modifier_flag modifier_flag) {
    pressed_modifier_flags_.insert(modifier_flag);
  }

  void erase_pressed_modifier_flag(modifier_flag modifier_flag) {
    pressed_modifier_flags_.erase(modifier_flag);
  }

private:
  bool key_event_exists(pqrs::hid::usage_page::value_t usage_page,
                        pqrs::hid::usage::value_t usage) {
    auto it = std::find_if(std::begin(pressed_keys_),
                           std::end(pressed_keys_),
                           [&](auto& k) {
                             return k.second.first == usage_page &&
                                    k.second.second == usage;
                           });
    return (it != std::end(pressed_keys_));
  }

  void enqueue_key_event(pqrs::hid::usage_page::value_t usage_page,
                         pqrs::hid::usage::value_t usage,
                         event_type event_type,
                         queue& queue,
                         absolute_time_point time_stamp) {
    queue.emplace_back_key_event(usage_page, usage, event_type, time_stamp);
  }

  std::vector<std::pair<device_id, std::pair<pqrs::hid::usage_page::value_t, pqrs::hid::usage::value_t>>> pressed_keys_;
  std::unordered_set<modifier_flag> pressed_modifier_flags_;
};
} // namespace post_event_to_virtual_devices
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
