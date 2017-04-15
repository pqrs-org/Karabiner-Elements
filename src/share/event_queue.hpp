#pragma once

#include "boost_defs.hpp"

#include "types.hpp"
#include <boost/optional.hpp>

namespace krbn {
class event_queue final {
public:
  class queued_event final {
  public:
    enum class type {
      key_code,
      pointing_button,
      pointing_x,
      pointing_y,
      pointing_vertical_wheel,
      pointing_horizontal_wheel,
    };

    queued_event(uint64_t time_stamp,
                 key_code key_code,
                 event_type event_type) : time_stamp_(time_stamp),
                                          type_(type::key_code),
                                          valid_(true),
                                          lazy_(false),
                                          key_code_(key_code),
                                          event_type_(event_type) {
    }

    queued_event(uint64_t time_stamp,
                 pointing_button pointing_button,
                 event_type event_type) : time_stamp_(time_stamp),
                                          type_(type::pointing_button),
                                          valid_(true),
                                          lazy_(false),
                                          pointing_button_(pointing_button),
                                          event_type_(event_type) {
    }

    queued_event(uint64_t time_stamp,
                 type type,
                 int integer_value) : time_stamp_(time_stamp),
                                      type_(type),
                                      valid_(true),
                                      lazy_(false),
                                      integer_value_(integer_value),
                                      event_type_(event_type::key_down) {
    }

    uint64_t get_time_stamp(void) const {
      return time_stamp_;
    }

    type get_type(void) const {
      return type_;
    }

    bool get_valid(void) const {
      return valid_;
    }
    void set_valid(bool value) {
      valid_ = value;
    }

    bool get_lazy(void) const {
      return lazy_;
    }
    void set_lazy(bool value) {
      lazy_ = value;
    }

    boost::optional<key_code> get_key_code(void) const {
      if (type_ == type::key_code) {
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
      return get_time_stamp() == other.get_time_stamp() &&
             get_type() == other.get_type() &&
             get_key_code() == other.get_key_code() &&
             get_pointing_button() == other.get_pointing_button() &&
             get_event_type() == other.get_event_type();
    }

  private:
    uint64_t time_stamp_;
    type type_;
    bool valid_;
    bool lazy_;

    union {
      key_code key_code_;               // For type::key_code
      pointing_button pointing_button_; // For type::pointing_button
      int integer_value_;               // For type::pointing_x, type::pointing_y, type::pointing_vertical_wheel, type::pointing_horizontal_wheel
    };
    event_type event_type_;
  };

  void push_back_event(uint64_t time_stamp,
                       hid_usage_page usage_page,
                       hid_usage usage,
                       int integer_value) {
    if (auto key_code = types::get_key_code(usage_page, usage)) {
      push_back_event(time_stamp,
                      *key_code,
                      integer_value ? event_type::key_down : event_type::key_up);
    } else if (auto pointing_button = types::get_pointing_button(usage_page, usage)) {
      push_back_event(time_stamp,
                      *pointing_button,
                      integer_value ? event_type::key_down : event_type::key_up);
    } else {
      switch (usage_page) {
      case hid_usage_page::generic_desktop:
        switch (usage) {
        case hid_usage::gd_x:
          push_back_event(time_stamp,
                          queued_event::type::pointing_x,
                          integer_value);
          break;

        case hid_usage::gd_y:
          push_back_event(time_stamp,
                          queued_event::type::pointing_y,
                          integer_value);
          break;

        case hid_usage::gd_wheel:
          push_back_event(time_stamp,
                          queued_event::type::pointing_vertical_wheel,
                          integer_value);
          break;

        default:
          break;
        }
        break;

      case hid_usage_page::consumer:
        switch (usage) {
        case hid_usage::csmr_acpan:
          push_back_event(time_stamp,
                          queued_event::type::pointing_horizontal_wheel,
                          integer_value);
          break;

        default:
          break;
        }
        break;

      default:
        break;
      }
    }
  }

  // For type::key_code
  void push_back_event(uint64_t time_stamp,
                       key_code key_code,
                       event_type event_type) {
    events_.emplace_back(time_stamp, key_code, event_type);
    sort_events();
  }

  // For type::pointing_button
  void push_back_event(uint64_t time_stamp,
                       pointing_button pointing_button,
                       event_type event_type) {
    events_.emplace_back(time_stamp, pointing_button, event_type);
    sort_events();
  }

  // For type::pointing_x, type::pointing_y, type::pointing_vertical_wheel, type::pointing_horizontal_wheel
  void push_back_event(uint64_t time_stamp,
                       queued_event::type type,
                       int integer_value) {
    events_.emplace_back(time_stamp, type, integer_value);
    sort_events();
  }

  const std::vector<queued_event>& get_events(void) {
    return events_;
  }

  static bool compare(const queued_event& v1, const queued_event& v2) {
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

    if (v1.get_time_stamp() == v2.get_time_stamp()) {
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
    return v1.get_time_stamp() < v2.get_time_stamp();
  }

private:
  void sort_events(void) {
    std::stable_sort(std::begin(events_), std::end(events_), event_queue::compare);
  }

  std::vector<queued_event> events_;
};
} // namespace krbn
