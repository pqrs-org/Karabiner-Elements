#pragma once

#include "../../../types.hpp"
#include "events_at_key_up.hpp"
#include "from_event.hpp"
#include <unordered_set>
#include <vector>

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace basic {
namespace manipulated_original_event {
class manipulated_original_event final {
public:
  manipulated_original_event(const std::unordered_set<from_event>& from_events,
                             const std::unordered_set<modifier_flag>& from_mandatory_modifiers,
                             absolute_time_point key_down_time_stamp) : from_events_(from_events),
                                                                        from_mandatory_modifiers_(from_mandatory_modifiers),
                                                                        key_down_time_stamp_(key_down_time_stamp),
                                                                        alone_(true),
                                                                        halted_(false),
                                                                        key_up_posted_(false) {
  }

  const std::unordered_set<from_event>& get_from_events(void) const {
    return from_events_;
  }

  const std::unordered_set<modifier_flag>& get_from_mandatory_modifiers(void) const {
    return from_mandatory_modifiers_;
  }

  std::unordered_set<modifier_flag>& get_key_up_posted_from_mandatory_modifiers(void) {
    return key_up_posted_from_mandatory_modifiers_;
  }

  absolute_time_point get_key_down_time_stamp(void) const {
    return key_down_time_stamp_;
  }

  bool get_alone(void) const {
    return alone_;
  }

  bool get_halted(void) const {
    return halted_;
  }

  void set_halted(void) {
    halted_ = true;
  }

  const events_at_key_up& get_events_at_key_up(void) const {
    return events_at_key_up_;
  }
  events_at_key_up& get_events_at_key_up(void) {
    return const_cast<events_at_key_up&>(static_cast<const manipulated_original_event&>(*this).get_events_at_key_up());
  }

  bool get_key_up_posted(void) const {
    return key_up_posted_;
  }

  void set_key_up_posted(bool value) {
    key_up_posted_ = true;
  }

  void unset_alone(void) {
    alone_ = false;
  }

  bool from_event_exists(const from_event& from_event) const {
    return from_events_.find(from_event) != std::end(from_events_);
  }

  void erase_from_event(const from_event& from_event) {
    from_events_.erase(from_event);
  }

  void erase_from_events_by_device_id(device_id device_id) {
    for (auto it = std::begin(from_events_); it != std::end(from_events_);) {
      if (it->get_device_id() == device_id) {
        it = from_events_.erase(it);
      } else {
        std::advance(it, 1);
      }
    }
  }

  void erase_from_events_by_event(const event_queue::event& event) {
    for (auto it = std::begin(from_events_); it != std::end(from_events_);) {
      if (it->get_event() == event) {
        it = from_events_.erase(it);
      } else {
        std::advance(it, 1);
      }
    }
  }

private:
  std::unordered_set<from_event> from_events_;
  std::unordered_set<modifier_flag> from_mandatory_modifiers_;
  std::unordered_set<modifier_flag> key_up_posted_from_mandatory_modifiers_;
  absolute_time_point key_down_time_stamp_;
  bool alone_;
  bool halted_;
  events_at_key_up events_at_key_up_;
  bool key_up_posted_;
};
} // namespace manipulated_original_event
} // namespace basic
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
