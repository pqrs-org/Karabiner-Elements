#pragma once

// `krbn::grabbable_state_queues_manager` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include "device_detail.hpp"
#include "event_queue.hpp"
#include "grabbable_state_queue.hpp"
#include <boost/optional.hpp>
#include <boost/signals2.hpp>
#include <mutex>
#include <unordered_map>

namespace krbn {
class grabbable_state_queues_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals

  boost::signals2::signal<void(registry_entry_id, boost::optional<grabbable_state>)> grabbable_state_changed;

  // Methods

  grabbable_state_queues_manager(const grabbable_state_queues_manager&) = delete;

  grabbable_state_queues_manager(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher) : dispatcher_client(weak_dispatcher) {
  }

  virtual ~grabbable_state_queues_manager(void) {
    detach_from_dispatcher([] {
    });
  }

  boost::optional<grabbable_state> find_current_grabbable_state(registry_entry_id registry_entry_id) const {
    std::lock_guard<std::mutex> lock(queues_mutex_);

    auto it = queues_.find(registry_entry_id);
    if (it != std::end(queues_)) {
      return it->second->find_current_grabbable_state();
    }

    return boost::none;
  }

  void clear(void) {
    std::lock_guard<std::mutex> lock(queues_mutex_);

    queues_.clear();
  }

  bool update_grabbable_state(const grabbable_state& state) {
    auto queue = find_or_create_queue(state.get_registry_entry_id());
    return queue->push_back_grabbable_state(state);
  }

  bool update_first_grabbed_event_time_stamp(const event_queue::queue& event_queue) {
    bool result = false;

    for (const auto& entry : event_queue.get_entries()) {
      if (auto device_detail = types::find_device_detail(entry.get_device_id())) {
        if (auto registry_entry_id = device_detail->get_registry_entry_id()) {
          auto queue = find_or_create_queue(*registry_entry_id);
          auto time_stamp = entry.get_event_time_stamp().get_time_stamp();
          if (queue->update_first_grabbed_event_time_stamp(time_stamp)) {
            result = true;
          }
        }
      }
    }

    return result;
  }

  void unset_first_grabbed_event_time_stamp(registry_entry_id registry_entry_id) {
    auto queue = find_or_create_queue(registry_entry_id);
    queue->unset_first_grabbed_event_time_stamp();
  }

  void erase_queue(registry_entry_id registry_entry_id) {
    std::lock_guard<std::mutex> lock(queues_mutex_);

    queues_.erase(registry_entry_id);
  }

private:
  std::shared_ptr<grabbable_state_queue> find_or_create_queue(registry_entry_id registry_entry_id) {
    std::lock_guard<std::mutex> lock(queues_mutex_);

    auto it = queues_.find(registry_entry_id);
    if (it != std::end(queues_)) {
      return it->second;
    }

    auto queue = std::make_shared<grabbable_state_queue>(weak_dispatcher_);
    queue->grabbable_state_changed.connect([this, registry_entry_id](auto&& grabbable_state) {
      grabbable_state_changed(registry_entry_id, grabbable_state);
    });

    queues_[registry_entry_id] = queue;
    return queue;
  }

  std::unordered_map<registry_entry_id, std::shared_ptr<grabbable_state_queue>> queues_;
  mutable std::mutex queues_mutex_;
};
} // namespace krbn
