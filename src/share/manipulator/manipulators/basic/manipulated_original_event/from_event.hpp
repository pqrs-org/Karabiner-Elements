#pragma once

#include "event_queue.hpp"
#include <pqrs/hash.hpp>

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace basic {
namespace manipulated_original_event {
class from_event final {
public:
  from_event(void) : device_id_(device_id(0)) {
  }

  from_event(device_id device_id,
             const event_queue::event& event,
             const event_queue::event& original_event) : device_id_(device_id),
                                                         event_(event),
                                                         original_event_(original_event) {
  }

  device_id get_device_id(void) const {
    return device_id_;
  }

  const event_queue::event& get_event(void) const {
    return event_;
  }

  const event_queue::event& get_original_event(void) const {
    return original_event_;
  }

  bool operator==(const from_event& other) const {
    return device_id_ == other.device_id_ &&
           event_ == other.event_ &&
           original_event_ == other.original_event_;
  }

private:
  device_id device_id_;
  event_queue::event event_;
  event_queue::event original_event_;
};
} // namespace manipulated_original_event
} // namespace basic
} // namespace manipulators
} // namespace manipulator
} // namespace krbn

namespace std {
using krbn::manipulator::manipulators::basic::manipulated_original_event::from_event;

template <>
struct hash<from_event> final {
  std::size_t operator()(const from_event& value) const {
    std::size_t h = 0;

    pqrs::hash::combine(h, value.get_device_id());
    pqrs::hash::combine(h, value.get_event());
    pqrs::hash::combine(h, value.get_original_event());

    return h;
  }
};
} // namespace std
