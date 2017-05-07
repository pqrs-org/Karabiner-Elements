#pragma once

#include "event_queue.hpp"
#include "manipulator/modifier_flag_manager.hpp"

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

  virtual void manipulate(event_queue& event_queue,
                          size_t previous_events_size,
                          modifier_flag_manager& modifier_flag_manager,
                          uint64_t time_stamp) = 0;

  virtual bool active(void) const = 0;

  virtual void inactivate_by_device_id(device_id device_id,
                                       event_queue& event_queue,
                                       modifier_flag_manager& modifier_flag_manager,
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
