#pragma once

#include "boost_defs.hpp"

#include "device_detail.hpp"
#include "event_queue.hpp"
#include "keyboard_repeat_detector.hpp"
#include <boost/signals2.hpp>

namespace krbn {
class grabbable_state_manager final {
public:
#include "grabbable_state_manager/entry.hpp"

  boost::signals2::signal<void(registry_entry_id,
                               grabbable_state,
                               ungrabbable_temporarily_reason,
                               uint64_t)>
      grabbable_state_updated;

  void update(const event_queue& event_queue) {
    for (const auto& queued_event : event_queue.get_events()) {
      if (auto device_detail = types::find_device_detail(queued_event.get_device_id())) {
        if (auto registry_entry_id = device_detail->get_registry_entry_id()) {
          auto it = entries_.find(*registry_entry_id);
          if (it == std::end(entries_)) {
            entries_.emplace(*registry_entry_id, entry{});
            it = entries_.find(*registry_entry_id);
          }

          if (it != std::end(entries_)) {
            auto old_state = it->second.get_grabbable_state();

            it->second.update(queued_event);

            auto new_state = it->second.get_grabbable_state();

            if (old_state != new_state) {
              grabbable_state_updated(*registry_entry_id,
                                      new_state.first,
                                      new_state.second,
                                      queued_event.get_event_time_stamp().get_time_stamp());
            }
          }
        }
      }
    }
  }

  void erase(registry_entry_id registry_entry_id) {
    entries_.erase(registry_entry_id);
  }

  boost::optional<std::pair<grabbable_state, ungrabbable_temporarily_reason>> get_grabbable_state(registry_entry_id registry_entry_id) {
    auto it = entries_.find(registry_entry_id);
    if (it != std::end(entries_)) {
      return it->second.get_grabbable_state();
    }
    return boost::none;
  }

private:
  std::unordered_map<registry_entry_id, entry> entries_;
};
} // namespace krbn
