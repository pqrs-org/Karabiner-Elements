#pragma once

#include "pressed_keys_manager.hpp"
#include "queue.hpp"

namespace krbn {
namespace event_queue {
namespace utility {
static inline std::shared_ptr<queue> insert_device_keys_and_pointing_buttons_are_released_event(std::shared_ptr<queue> queue,
                                                                                                device_id device_id,
                                                                                                std::shared_ptr<pressed_keys_manager> pressed_keys_manager) {
  auto result = std::make_shared<event_queue::queue>();

  if (queue && pressed_keys_manager) {
    for (const auto& entry : queue->get_entries()) {
      result->push_back_event(entry);

      if (entry.get_device_id() == device_id) {
        auto& e = entry.get_event();

        if (entry.get_event_type() == event_type::key_down) {
          if (auto v = e.find<key_code>()) {
            pressed_keys_manager->insert(*v);
          } else if (auto v = e.find<consumer_key_code>()) {
            pressed_keys_manager->insert(*v);
          } else if (auto v = e.find<pointing_button>()) {
            pressed_keys_manager->insert(*v);
          }
        } else if (entry.get_event_type() == event_type::key_up) {
          if (!pressed_keys_manager->empty()) {
            if (auto v = e.find<key_code>()) {
              pressed_keys_manager->erase(*v);
            } else if (auto v = e.find<consumer_key_code>()) {
              pressed_keys_manager->erase(*v);
            } else if (auto v = e.find<pointing_button>()) {
              pressed_keys_manager->erase(*v);
            }

            if (pressed_keys_manager->empty()) {
              auto event = event::make_device_keys_and_pointing_buttons_are_released_event();
              result->emplace_back_event(device_id,
                                         entry.get_event_time_stamp(),
                                         event,
                                         event_type::single,
                                         event);
            }
          }
        }
      }
    }
  }

  return result;
}
} // namespace utility
} // namespace event_queue
} // namespace krbn
