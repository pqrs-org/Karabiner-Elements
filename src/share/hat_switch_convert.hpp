#pragma once

#include "types/device_id.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace krbn {
class hat_switch_converter final {
public:
  typedef enum {
    up = 0x1 << 0,
    down = 0x1 << 1,
    right = 0x1 << 2,
    left = 0x1 << 3
  } direction;

  static std::shared_ptr<hat_switch_converter> get_global_hat_switch_converter(void) {
    if (!hat_switch_converter_) {
      hat_switch_converter_ = std::make_shared<hat_switch_converter>();
    }

    return hat_switch_converter_;
  }

  void erase_device(device_id device_id) {
    last_directions_.erase(device_id);
  }

  void clear(void) {
    last_directions_.clear();
  }

  std::vector<std::pair<momentary_switch_event, event_type>> to_dpad_events(device_id device_id,
                                                                            CFIndex hat_switch_value) {
    std::vector<std::pair<momentary_switch_event, event_type>> result;

    auto last_direction = direction(0);
    auto it = last_directions_.find(device_id);
    if (it != std::end(last_directions_)) {
      last_direction = it->second;
    }

    auto current_direction = direction(0);
    switch (hat_switch_value) {
      case 0:
        current_direction = up;
        break;
      case 1:
        current_direction = direction(up | right);
        break;
      case 2:
        current_direction = right;
        break;
      case 3:
        current_direction = direction(down | right);
        break;
      case 4:
        current_direction = down;
        break;
      case 5:
        current_direction = direction(down | left);
        break;
      case 6:
        current_direction = left;
        break;
      case 7:
        current_direction = direction(up | left);
        break;
    }

    for (const auto& d : {
             std::make_pair(up, pqrs::hid::usage::generic_desktop::dpad_up),
             std::make_pair(down, pqrs::hid::usage::generic_desktop::dpad_down),
             std::make_pair(right, pqrs::hid::usage::generic_desktop::dpad_right),
             std::make_pair(left, pqrs::hid::usage::generic_desktop::dpad_left),
         }) {
      if (!(last_direction & d.first) && (current_direction & d.first)) {
        // key_down
        result.push_back(std::make_pair(momentary_switch_event(pqrs::hid::usage_page::generic_desktop, d.second),
                                        event_type::key_down));
      }

      if ((last_direction & d.first) && !(current_direction & d.first)) {
        // key_up
        result.push_back(std::make_pair(momentary_switch_event(pqrs::hid::usage_page::generic_desktop, d.second),
                                        event_type::key_up));
      }
    }

    last_directions_[device_id] = current_direction;

    return result;
  }

private:
  static inline std::shared_ptr<hat_switch_converter> hat_switch_converter_;

  std::unordered_map<device_id, direction> last_directions_;
};
} // namespace krbn
