#pragma once

#include "types.hpp"
#include <optional>
#include <utility>

namespace krbn {
class keyboard_repeat_detector final {
public:
  std::optional<std::pair<hid_usage_page, hid_usage>> get_repeating_key(void) const {
    return repeating_key_;
  }

  void set(hid_usage_page hid_usage_page,
           hid_usage hid_usage,
           event_type event_type) {
    switch (event_type) {
      case event_type::key_down:
        if (make_modifier_flag(hid_usage_page, hid_usage) == std::nullopt) {
          repeating_key_ = std::make_pair(hid_usage_page, hid_usage);
        }
        break;

      case event_type::key_up:
        if (repeating_key_ &&
            repeating_key_->first == hid_usage_page &&
            repeating_key_->second == hid_usage) {
          repeating_key_ = std::nullopt;
        }
        break;

      case event_type::single:
        // Do nothing
        break;
    }
  }

  void set(const hid_value& hid_value) {
    if (auto hid_usage_page = hid_value.get_hid_usage_page()) {
      if (auto hid_usage = hid_value.get_hid_usage()) {
        set(*hid_usage_page,
            *hid_usage,
            hid_value.get_integer_value() ? event_type::key_down : event_type::key_up);
      }
    }
  }

  void clear(void) {
    repeating_key_ = std::nullopt;
  }

  bool is_repeating(void) const {
    return repeating_key_ != std::nullopt;
  }

private:
  std::optional<std::pair<hid_usage_page, hid_usage>> repeating_key_;
};
} // namespace krbn
