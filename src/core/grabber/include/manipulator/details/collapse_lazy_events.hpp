#pragma once

#include "manipulator/details/base.hpp"
#include <vector>

namespace krbn {
namespace manipulator {
namespace details {
class collapse_lazy_events : public base {
public:
  collapse_lazy_events(void) : base() {
  }

  virtual ~collapse_lazy_events(void) {
  }

  virtual void manipulate(event_queue::queued_event& front_input_event,
                          const event_queue& input_event_queue,
                          event_queue& output_event_queue,
                          uint64_t time_stamp) {
    if (!front_input_event.get_valid()) {
      return;
    }

    // Invalidate if front_input_event is in invalidation_reserved_events_.

    for (auto it = std::begin(invalidation_reserved_events_); it != std::end(invalidation_reserved_events_); std::advance(it, 1)) {
      if (front_input_event == *it) {
        invalidation_reserved_events_.erase(it);
        front_input_event.set_valid(false);
        return;
      }
    }

    // Determine front_input_event should be collapsed.

    if (!front_input_event.get_lazy() ||
        front_input_event.get_event_type() != event_type::key_down) {
      return;
    }

    for (const auto& e : input_event_queue.get_events()) {
      if (!e.get_valid()) {
        continue;
      }

      if (e.get_event_type() == event_type::key_up &&
          e.get_event() == front_input_event.get_event()) {
        front_input_event.set_valid(false);
        invalidation_reserved_events_.push_back(e);
        return;
      }

      if (e.get_lazy() == false) {
        return;
      }
    }
  }

  virtual bool active(void) const {
    return false;
  }

  virtual void device_ungrabbed_callback(device_id device_id,
                                         event_queue& output_event_queue,
                                         uint64_t time_stamp) {
    invalidation_reserved_events_.erase(std::remove_if(std::begin(invalidation_reserved_events_),
                                                       std::end(invalidation_reserved_events_),
                                                       [&](auto& e) {
                                                         return device_id == e.get_device_id();
                                                       }),
                                        std::end(invalidation_reserved_events_));
  }

private:
  std::vector<event_queue::queued_event> invalidation_reserved_events_;
};
} // namespace details
} // namespace manipulator
} // namespace krbn
