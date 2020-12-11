#pragma once

#include "types.hpp"
#include <optional>
#include <utility>

namespace krbn {
class keyboard_repeat_detector final {
public:
  std::optional<momentary_switch_event> get_repeating_key(void) const {
    return repeating_key_;
  }

  void set(const momentary_switch_event& momentary_switch_event,
           event_type event_type) {
    if (!momentary_switch_event.valid()) {
      return;
    }

    switch (event_type) {
      case event_type::key_down: {
        if (!momentary_switch_event.modifier_flag()) {
          repeating_key_ = momentary_switch_event;
        }
        break;
      }

      case event_type::key_up:
        if (repeating_key_ == momentary_switch_event) {
          repeating_key_ = std::nullopt;
        }
        break;

      case event_type::single:
        // Do nothing
        break;
    }
  }

  void clear(void) {
    repeating_key_ = std::nullopt;
  }

  bool is_repeating(void) const {
    return repeating_key_ != std::nullopt;
  }

private:
  std::optional<momentary_switch_event> repeating_key_;
};
} // namespace krbn
