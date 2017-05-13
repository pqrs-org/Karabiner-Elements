#pragma once

#include "event_queue.hpp"
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

  virtual void manipulate(event_queue::queued_event& front_input_event,
                          const event_queue& input_event_queue,
                          event_queue& output_event_queue,
                          uint64_t time_stamp) = 0;

  virtual bool active(void) const = 0;

  // Manipulators should send key_up events in this method.
  virtual void device_ungrabbed_callback(device_id device_id,
                                         event_queue& output_event_queue,
                                         uint64_t time_stamp) = 0;

  bool get_valid(void) const {
    return valid_;
  }
  void set_valid(bool value) {
    valid_ = value;
  }

protected:
  bool valid_;
};
} // namespace details
} // namespace manipulator
} // namespace krbn
