#pragma once

// `krbn::grabbable_state_manager` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include "device_detail.hpp"
#include "event_queue.hpp"
#include "keyboard_repeat_detector.hpp"
#include <boost/signals2.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/iokit_types.hpp>

namespace krbn {
class grabbable_state_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
#include "grabbable_state_manager/entry.hpp"

  // Signals (invoked from the shared dispatcher thread)

  boost::signals2::signal<void(const grabbable_state&)> grabbable_state_changed;

  // Methods

  grabbable_state_manager(const grabbable_state_manager&) = delete;

  grabbable_state_manager(void) : dispatcher_client() {
  }

  virtual ~grabbable_state_manager(void) {
    detach_from_dispatcher([] {
    });
  }

  void update(const grabbable_state& state) {
    std::lock_guard<std::mutex> lock(entries_mutex_);

    auto registry_entry_id = state.get_registry_entry_id();

    auto it = entries_.find(registry_entry_id);
    if (it == std::end(entries_)) {
      entries_.emplace(registry_entry_id,
                       entry(registry_entry_id));
      it = entries_.find(registry_entry_id);
    }

    if (it != std::end(entries_)) {
      auto old_state = it->second.get_grabbable_state();

      it->second.set_grabbable_state(state);

      auto new_state = it->second.get_grabbable_state();

      if (!old_state.equals_except_time_stamp(new_state)) {
        enqueue_to_dispatcher([this, new_state] {
          grabbable_state_changed(new_state);
        });
      }
    }
  }

  void update(const event_queue::queue& event_queue) {
    std::lock_guard<std::mutex> lock(entries_mutex_);

    for (const auto& event_queue_entry : event_queue.get_entries()) {
      if (auto device_detail = types::find_device_detail(event_queue_entry.get_device_id())) {
        if (auto registry_entry_id = device_detail->get_registry_entry_id()) {
          auto time_stamp = event_queue_entry.get_event_time_stamp().get_time_stamp();

          auto it = entries_.find(*registry_entry_id);
          if (it == std::end(entries_)) {
            entries_.emplace(*registry_entry_id,
                             entry(*registry_entry_id));
            it = entries_.find(*registry_entry_id);
          }

          if (it != std::end(entries_)) {
            auto old_state = it->second.get_grabbable_state();

            it->second.update(*registry_entry_id,
                              time_stamp,
                              event_queue_entry);

            auto new_state = it->second.get_grabbable_state();

            if (!old_state.equals_except_time_stamp(new_state)) {
              enqueue_to_dispatcher([this, new_state] {
                grabbable_state_changed(new_state);
              });
            }
          }
        }
      }
    }
  }

  void erase(pqrs::osx::iokit_registry_entry_id registry_entry_id) {
    std::lock_guard<std::mutex> lock(entries_mutex_);

    entries_.erase(registry_entry_id);
  }

  boost::optional<grabbable_state> get_grabbable_state(pqrs::osx::iokit_registry_entry_id registry_entry_id) const {
    std::lock_guard<std::mutex> lock(entries_mutex_);

    auto it = entries_.find(registry_entry_id);
    if (it != std::end(entries_)) {
      return it->second.get_grabbable_state();
    }
    return boost::none;
  }

private:
  std::unordered_map<pqrs::osx::iokit_registry_entry_id, entry> entries_;
  mutable std::mutex entries_mutex_;
};
} // namespace krbn
