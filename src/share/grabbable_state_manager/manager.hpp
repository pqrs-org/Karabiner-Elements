#pragma once

// `krbn::grabbable_state_manager::manager` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include "grabbable_state_manager/entry.hpp"
#include <boost/signals2.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/iokit_types.hpp>

namespace krbn {
namespace grabbable_state_manager {
class manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  boost::signals2::signal<void(const grabbable_state&)> grabbable_state_changed;

  // Methods

  manager(const manager&) = delete;

  manager(void) : dispatcher_client() {
  }

  virtual ~manager(void) {
    detach_from_dispatcher([] {
    });
  }

  void update(const grabbable_state& state) {
    std::lock_guard<std::mutex> lock(entries_mutex_);

    auto device_id = state.get_device_id();

    auto it = entries_.find(device_id);
    if (it == std::end(entries_)) {
      entries_.emplace(device_id,
                       entry(device_id));
      it = entries_.find(device_id);
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
      auto device_id = event_queue_entry.get_device_id();
      auto time_stamp = event_queue_entry.get_event_time_stamp().get_time_stamp();

      auto it = entries_.find(device_id);
      if (it == std::end(entries_)) {
        entries_.emplace(device_id,
                         entry(device_id));
        it = entries_.find(device_id);
      }

      if (it != std::end(entries_)) {
        auto old_state = it->second.get_grabbable_state();

        it->second.update(device_id,
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

  void erase(device_id device_id) {
    std::lock_guard<std::mutex> lock(entries_mutex_);

    entries_.erase(device_id);
  }

  std::optional<grabbable_state> get_grabbable_state(device_id device_id) const {
    std::lock_guard<std::mutex> lock(entries_mutex_);

    auto it = entries_.find(device_id);
    if (it != std::end(entries_)) {
      return it->second.get_grabbable_state();
    }
    return std::nullopt;
  }

private:
  std::unordered_map<device_id, entry> entries_;
  mutable std::mutex entries_mutex_;
};
} // namespace grabbable_state_manager
} // namespace krbn
