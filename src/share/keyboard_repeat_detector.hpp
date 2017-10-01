#pragma once

#include "boost_defs.hpp"

#include "types.hpp"
#include <boost/optional.hpp>
#include <utility>

namespace krbn {
class keyboard_repeat_detector final {
public:
  void set(hid_usage_page hid_usage_page,
           hid_usage hid_usage,
           event_type event_type) {
    switch (event_type) {
      case event_type::key_down:
        if (types::make_modifier_flag(hid_usage_page, hid_usage) == modifier_flag::zero) {
          repeating_key_ = std::make_pair(hid_usage_page, hid_usage);
        }
        break;

      case event_type::key_up:
        if (repeating_key_ &&
            repeating_key_->first == hid_usage_page &&
            repeating_key_->second == hid_usage) {
          repeating_key_ = boost::none;
        }
        break;

      case event_type::single:
        // Do nothing
        break;
    }
  }

  bool is_repeating(void) const {
    return repeating_key_ != boost::none;
  }

private:
  boost::optional<std::pair<hid_usage_page, hid_usage>> repeating_key_;
};
} // namespace krbn
