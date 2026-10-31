#pragma once

#include "event_queue.hpp"

namespace krbn::manipulator::manipulators::basic::manipulated_original_event {
class events_at_key_up final {
public:
  class entry {
  public:
    entry(device_id device_id,
          const event_queue::event& event,
          event_type event_type,
          const event_queue::event& original_event,
          bool lazy,
          bool release_when_other_key_is_pressed) : device_id_(device_id),
                                                    event_(event),
                                                    event_type_(event_type),
                                                    original_event_(original_event),
                                                    lazy_(lazy),
                                                    release_when_other_key_is_pressed_(release_when_other_key_is_pressed) {
    }

    event_queue::entry make_entry(const event_queue::event_time_stamp& event_time_stamp,
                                  bool force_non_lazy = false) const {
      return event_queue::entry(device_id_,
                                event_time_stamp,
                                event_,
                                event_type_,
                                std::nullopt,
                                original_event_,
                                event_queue::state::manipulated,
                                force_non_lazy ? false : lazy_);
    }

    [[nodiscard]] bool get_release_when_other_key_is_pressed() const {
      return release_when_other_key_is_pressed_;
    }

  private:
    device_id device_id_;
    event_queue::event event_;
    event_type event_type_;
    event_queue::event original_event_;
    bool lazy_;
    bool release_when_other_key_is_pressed_;
  };

  [[nodiscard]] const std::vector<entry>& get_events() const {
    return events_;
  }

  void emplace_back_event(device_id device_id,
                          const event_queue::event& event,
                          event_type event_type,
                          const event_queue::event& original_event,
                          bool lazy,
                          bool release_when_other_key_is_pressed = false) {
    events_.emplace_back(device_id,
                         event,
                         event_type,
                         original_event,
                         lazy,
                         release_when_other_key_is_pressed);
  }

  std::vector<entry> extract_events_when_other_key_is_pressed() {
    std::vector<entry> result;

    auto it = std::begin(events_);
    while (it != std::end(events_)) {
      if (it->get_release_when_other_key_is_pressed()) {
        result.push_back(*it);
        it = events_.erase(it);
      } else {
        ++it;
      }
    }

    return result;
  }

  void clear_events() {
    events_.clear();
  }

private:
  std::vector<entry> events_;
};
} // namespace krbn::manipulator::manipulators::basic::manipulated_original_event
