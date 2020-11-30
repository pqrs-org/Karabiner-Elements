#pragma once

#include "device_properties.hpp"
#include "event_queue.hpp"
#include "manipulator/manipulator_environment.hpp"
#include "types/device_id.hpp"
#include "types/product_id.hpp"
#include "types/vendor_id.hpp"

namespace krbn {
namespace unit_testing {
class manipulator_conditions_helper final {
public:
  manipulator_conditions_helper(void) : last_device_id_(0) {
  }

  const krbn::manipulator::manipulator_environment& get_manipulator_environment(void) {
    return manipulator_environment_;
  }

  krbn::device_id prepare_device(std::optional<pqrs::hid::vendor_id::value_t> vendor_id,
                                 std::optional<pqrs::hid::product_id::value_t> product_id,
                                 std::optional<krbn::location_id> location_id,
                                 std::optional<bool> is_keyboard,
                                 std::optional<bool> is_pointing_device) {
    ++last_device_id_;
    krbn::device_id device_id(last_device_id_);

    auto properties = krbn::device_properties().set(device_id);
    if (vendor_id) {
      properties.set(*vendor_id);
    }
    if (product_id) {
      properties.set(*product_id);
    }
    if (location_id) {
      properties.set(*location_id);
    }
    if (is_keyboard) {
      properties.set_is_keyboard(*is_keyboard);
    }
    if (is_pointing_device) {
      properties.set_is_pointing_device(*is_pointing_device);
    }

    manipulator_environment_.insert_device_properties(device_id,
                                                      properties);

    return device_id;
  }

  krbn::event_queue::entry make_event_queue_entry(krbn::device_id device_id) const {
    return krbn::event_queue::entry(device_id,
                                    krbn::event_queue::event_time_stamp(krbn::absolute_time_point(0)),
                                    krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                          pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                    krbn::event_type::key_down,
                                    krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                          pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                    krbn::event_queue::state::original);
  };

private:
  krbn::manipulator::manipulator_environment manipulator_environment_;
  int last_device_id_;
};
} // namespace unit_testing
} // namespace krbn
