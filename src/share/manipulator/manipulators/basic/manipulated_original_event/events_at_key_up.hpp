#pragma once

#include "event_queue.hpp"

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace basic {
namespace manipulated_original_event {
class events_at_key_up final {
public:
  class entry {
  public:
    entry(device_id device_id,
          const event_queue::event& event,
          event_type event_type,
          const event_queue::event& original_event,
          bool lazy) : device_id_(device_id),
                       event_(event),
                       event_type_(event_type),
                       original_event_(original_event),
                       lazy_(lazy) {
    }

    event_queue::entry make_entry(const event_queue::event_time_stamp& event_time_stamp) const {
      return event_queue::entry(device_id_,
                                event_time_stamp,
                                event_,
                                event_type_,
                                original_event_,
                                event_queue::state::manipulated,
                                lazy_);
    }

  private:
    device_id device_id_;
    event_queue::event event_;
    event_type event_type_;
    event_queue::event original_event_;
    bool lazy_;
  };

  const std::vector<entry>& get_events(void) const {
    return events_;
  }

  void emplace_back_event(device_id device_id,
                          const event_queue::event& event,
                          event_type event_type,
                          const event_queue::event& original_event,
                          bool lazy) {
    events_.emplace_back(device_id,
                         event,
                         event_type,
                         original_event,
                         lazy);
  }

  void clear_events(void) {
    events_.clear();
  }

private:
  std::vector<entry> events_;
};
} // namespace manipulated_original_event
} // namespace basic
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
