#pragma once

#include "types.hpp"
#include <unordered_map>

namespace krbn {
class physical_keyboard_repeat_detector final {
public:
  void set(device_id device_id, key_code key_code, event_type event_type) {
    if (event_type == event_type::key_down) {
      if (types::get_modifier_flag(key_code) != modifier_flag::zero) {
        repeating_keys_.erase(device_id);
      } else {
        repeating_keys_[device_id] = key_code;
      }

    } else if (event_type == event_type::key_up) {
      auto it = repeating_keys_.find(device_id);
      if (it != std::end(repeating_keys_)) {
        if (it->second == key_code) {
          repeating_keys_.erase(device_id);
        }
      }
    }
  }

  void erase(device_id device_id) {
    repeating_keys_.erase(device_id);
  }

  bool is_repeating(device_id device_id) {
    return repeating_keys_.find(device_id) != std::end(repeating_keys_);
  }

private:
  std::unordered_map<device_id, key_code> repeating_keys_;
};
} // namespace krbn
