#pragma once

#include "manipulator/details/base.hpp"
#include "manipulator/details/types.hpp"
#include "types.hpp"
#include <mach/mach_time.h>

namespace krbn {
namespace manipulator {
namespace details {
class add_delay_after_modifier_key_down final : public base {
public:
  add_delay_after_modifier_key_down(void) : base() {
  }

  virtual ~add_delay_after_modifier_key_down(void) {
  }

  virtual void manipulate(event_queue::queued_event& front_input_event,
                          const event_queue& input_event_queue,
                          event_queue& output_event_queue,
                          uint64_t time_stamp) {
    output_event_queue.push_back_event(front_input_event);
    front_input_event.set_valid(false);

    if (auto key_code = front_input_event.get_event().get_key_code()) {
      auto m = types::get_modifier_flag(*key_code);
      if (m != modifier_flag::zero) {
        if (front_input_event.get_event_type() == event_type::key_down) {
          // wait 1 millisecond
          output_event_queue.increase_time_stamp_delay(NSEC_PER_MSEC);
        }
      }
    }
  }

  virtual bool active(void) const {
    return false;
  }

  virtual void device_ungrabbed_callback(device_id device_id,
                                         event_queue& output_event_queue,
                                         uint64_t time_stamp) {
    // Do nothing
  }
};
} // namespace details
} // namespace manipulator
} // namespace krbn
