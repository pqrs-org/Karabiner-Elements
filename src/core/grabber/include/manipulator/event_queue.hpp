#pragma once

#include "boost_defs.hpp"

#include "types.hpp"
#include <boost/optional.hpp>
#include <chrono>

namespace krbn {
namespace manipulator {
class event_queue final {
public:
  enum class scope {
    input,
    output,
  };

  class queued_event final {
  public:
    enum class type {
      key,
      pointing_button,
    };

    queued_event(scope scope,
                 std::chrono::nanoseconds time,
                 key_code key_code,
                 event_type event_type) : type_(type::key),
                                          scope_(scope),
                                          time_(time),
                                          key_code_(key_code),
                                          event_type_(event_type) {
    }

    type get_type(void) const {
      return type_;
    }

    scope get_scope(void) const {
      return scope_;
    }

    std::chrono::nanoseconds get_time(void) const {
      return time_;
    }

    boost::optional<key_code> get_key_code(void) const {
      if (type_ == type::key) {
        return key_code_;
      }
      return boost::none;
    }

    boost::optional<pointing_button> get_pointing_button(void) const {
      if (type_ == type::pointing_button) {
        return pointing_button_;
      }
      return boost::none;
    }

    event_type get_event_type(void) const {
      return event_type_;
    }

    bool operator==(const queued_event& other) const {
      return get_type() == other.get_type() &&
             get_scope() == other.get_scope() &&
             get_time() == other.get_time() &&
             get_key_code() == other.get_key_code() &&
             get_pointing_button() == other.get_pointing_button() &&
             get_event_type() == other.get_event_type();
    }

  private:
    type type_;
    scope scope_;
    std::chrono::nanoseconds time_;

    union {
      key_code key_code_;
      pointing_button pointing_button_;
    };
    event_type event_type_;
  };

  void push_back_event(scope scope,
                       std::chrono::nanoseconds time,
                       key_code key_code,
                       event_type event_type) {
    switch (scope) {
    case scope::input:
      input_events_.emplace_back(scope, time, key_code, event_type);
      sort_events(input_events_);
      break;
    case scope::output:
      output_events_.emplace_back(scope, time, key_code, event_type);
      sort_events(output_events_);
      break;
    }
  }

  const std::vector<queued_event>& get_input_events(void) const {
    return input_events_;
  }

  const std::vector<queued_event>& get_output_events(void) const {
    return output_events_;
  }

private:
  void sort_events(std::vector<queued_event>& events) {
    // Some devices are send modifier flag and key at the same HID report.
    // For example, a key sends control+up-arrow by this reports.
    //
    //   modifiers: 0x0
    //   keys: 0x0 0x0 0x0 0x0 0x0 0x0
    //
    //   modifiers: 0x1
    //   keys: 0x52 0x0 0x0 0x0 0x0 0x0
    //
    // In this case, macOS does not guarantee the value event order to be modifier first.
    // At least macOS 10.12 or prior sends the up-arrow event first.
    //
    //   ----------------------------------------
    //   Example of hid value events in a single queue at control+up-arrow
    //
    //   1. up-arrow keydown
    //     usage_page:0x7
    //     usage:0x4f
    //     integer_value:1
    //
    //   2. control keydown
    //     usage_page:0x7
    //     usage:0xe1
    //     integer_value:1
    //
    //   3. up-arrow keyup
    //     usage_page:0x7
    //     usage:0x4f
    //     integer_value:0
    //
    //   4. control keyup
    //     usage_page:0x7
    //     usage:0xe1
    //     integer_value:0
    //   ----------------------------------------
    //
    // These events will not be interpreted as intended in this order.
    // Thus, we have to reorder the events.

    std::sort(std::begin(events), std::end(events), [](const queued_event& v1, const queued_event& v2) {
      if (v1.get_time() == v2.get_time()) {
        auto modifier_flag1 = modifier_flag::zero;
        auto modifier_flag2 = modifier_flag::zero;

        if (auto key_code1 = v1.get_key_code()) {
          modifier_flag1 = types::get_modifier_flag(*key_code1);
        }
        if (auto key_code2 = v2.get_key_code()) {
          modifier_flag2 = types::get_modifier_flag(*key_code2);
        }

        // If either modifier_flag1 or modifier_flag2 is modifier, reorder it before.

        if (modifier_flag1 == modifier_flag::zero &&
            modifier_flag2 != modifier_flag::zero) {
          // v2 is modifier_flag
          if (v2.get_event_type() == event_type::key_up) {
            return true;
          } else {
            // reorder to v2,v1 if v2 is pressed.
            return false;
          }
        }

        if (modifier_flag1 != modifier_flag::zero &&
            modifier_flag2 == modifier_flag::zero) {
          // v1 is modifier_flag
          if (v1.get_event_type() == event_type::key_up) {
            // reorder to v2,v1 if v1 is released.
            return false;
          } else {
            return true;
          }
        }
      }

      // keep order
      return v1.get_time() <= v2.get_time();
    });
  }

  std::vector<queued_event> input_events_;
  std::vector<queued_event> output_events_;
};
} // namespace manipulator
} // namespace krbn
