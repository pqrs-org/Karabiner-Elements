#pragma once

#include "manipulator/details/base.hpp"
#include "manipulator/details/types.hpp"

namespace krbn {
namespace manipulator {
namespace details {
class nop final : public base {
public:
  nop(void) : base() {
  }

  virtual ~nop(void) {
  }

  virtual void manipulate(event_queue::queued_event& front_input_event,
                          const event_queue& input_event_queue,
                          event_queue& output_event_queue,
                          uint64_t time_stamp) {
  }

  virtual bool active(void) const {
    return false;
  }

  virtual void device_ungrabbed_callback(device_id device_id,
                                         event_queue& output_event_queue,
                                         uint64_t time_stamp) {
  }
};
} // namespace details
} // namespace manipulator
} // namespace krbn
