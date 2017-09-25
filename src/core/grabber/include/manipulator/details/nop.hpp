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
                          event_queue& output_event_queue) {
  }

  virtual bool active(void) const {
    return false;
  }

  virtual bool needs_virtual_hid_pointing(void) const {
    return false;
  }

  virtual void handle_device_ungrabbed_event(device_id device_id,
                                             const event_queue& output_event_queue,
                                             uint64_t time_stamp) {
  }

  virtual void handle_event_from_ignored_device(const event_queue::queued_event& front_input_event,
                                                event_queue& output_event_queue) {
  }

  virtual void force_post_modifier_key_event(const event_queue::queued_event& front_input_event,
                                              event_queue& output_event_queue) {
  }

  virtual void force_post_pointing_button_event(const event_queue::queued_event& front_input_event,
                                                event_queue& output_event_queue) {
  }
};
} // namespace details
} // namespace manipulator
} // namespace krbn
