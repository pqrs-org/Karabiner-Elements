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

  virtual void handle_device_ungrabbed_event(device_id device_id,
                                             const event_queue& output_event_queue,
                                             uint64_t time_stamp) = 0;

  virtual void handle_event_from_ignored_device(event_queue::queued_event::event::type original_type,
                                                event_type event_type,
                                                event_queue& output_event_queue,
                                                uint64_t time_stamp) = 0;

  bool get_valid(void) const {
    return valid_;
  }
  virtual void set_valid(bool value) {
    valid_ = value;
  }

protected:
  bool valid_;
};
} // namespace details
} // namespace manipulator
} // namespace krbn
