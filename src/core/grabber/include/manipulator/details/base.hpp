#pragma once

#include "event_queue.hpp"
#include "manipulator/condition_manager.hpp"
#include "manipulator/types.hpp"
#include "modifier_flag_manager.hpp"

namespace krbn {
namespace manipulator {
namespace details {
class base {
protected:
  base(void) : valid_(true) {
  }

public:
  virtual ~base(void) {
  }

  virtual bool already_manipulated(const event_queue::entry& front_input_event) = 0;

  virtual manipulate_result manipulate(event_queue::entry& front_input_event,
                                       const event_queue::queue& input_event_queue,
                                       const std::shared_ptr<event_queue::queue>& output_event_queue,
                                       absolute_time_point now) = 0;

  virtual bool active(void) const = 0;

  virtual bool needs_virtual_hid_pointing(void) const = 0;

  virtual void handle_device_keys_and_pointing_buttons_are_released_event(const event_queue::entry& front_input_event,
                                                                          event_queue::queue& output_event_queue) = 0;

  virtual void handle_device_ungrabbed_event(device_id device_id,
                                             const event_queue::queue& output_event_queue,
                                             absolute_time_point time_stamp) = 0;

  virtual void handle_pointing_device_event_from_event_tap(const event_queue::entry& front_input_event,
                                                           event_queue::queue& output_event_queue) = 0;

  bool get_valid(void) const {
    return valid_;
  }
  virtual void set_valid(bool value) {
    valid_ = value;
  }

  void push_back_condition(const std::shared_ptr<manipulator::conditions::base>& condition) {
    condition_manager_.push_back_condition(condition);
  }

protected:
  bool valid_;
  condition_manager condition_manager_;
};
} // namespace details
} // namespace manipulator
} // namespace krbn
