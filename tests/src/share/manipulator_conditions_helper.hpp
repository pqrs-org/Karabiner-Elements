#pragma once

#include "device_properties.hpp"
#include "event_queue.hpp"
#include "manipulator/manipulator_environment.hpp"
#include "types/device_id.hpp"

namespace krbn {
namespace unit_testing {
class manipulator_conditions_helper final {
public:
  manipulator_conditions_helper(void) : last_device_id_(0) {
    core_configuration_ = std::make_shared<krbn::core_configuration::core_configuration>();
    manipulator_environment_.set_core_configuration(core_configuration_);
  }

  const krbn::manipulator::manipulator_environment& get_manipulator_environment(void) const {
    return manipulator_environment_;
  }

  krbn::manipulator::manipulator_environment& get_manipulator_environment(void) {
    return const_cast<krbn::manipulator::manipulator_environment&>(static_cast<const manipulator_conditions_helper&>(*this).get_manipulator_environment());
  }

  std::shared_ptr<krbn::core_configuration::core_configuration> get_core_configuration(void) const {
    return core_configuration_;
  }

  krbn::device_id prepare_device(const krbn::device_properties::initialization_parameters& parameters) {
    ++last_device_id_;

    auto p = parameters;
    p.device_id = device_id(last_device_id_);

    manipulator_environment_.insert_device_properties(p.device_id,
                                                      std::make_shared<krbn::device_properties>(p));

    return p.device_id;
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
  std::shared_ptr<krbn::core_configuration::core_configuration> core_configuration_;
  int last_device_id_;
};
} // namespace unit_testing
} // namespace krbn
