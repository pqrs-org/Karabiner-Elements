#pragma once

#include "device_detail.hpp"
#include "event_queue.hpp"
#include "grabbable_state_queue.hpp"
#include "shared_instance_provider.hpp"
#include <unordered_map>

namespace krbn {
class grabbable_state_queues_manager final : public shared_instance_provider<grabbable_state_queues_manager> {
public:
  std::shared_ptr<grabbable_state> find_current_grabbable_state(registry_entry_id registry_entry_id) const {
    auto it = queues_.find(registry_entry_id);
    if (it != std::end(queues_)) {
      return it->second->find_current_grabbable_state();
    }

    return nullptr;
  }

  void clear(void) {
    queues_.clear();
  }

  void update_grabbable_state(std::shared_ptr<grabbable_state> state) {
    if (!state) {
      return;
    }

    auto queue = find_or_create_queue(state->get_registry_entry_id());
    queue->push_back_grabbable_state(state);
  }

  void update_first_grabbed_event_time_stamp(const event_queue& event_queue) {
    for (const auto& queued_event : event_queue.get_events()) {
      if (auto device_detail = types::find_device_detail(queued_event.get_device_id())) {
        if (auto registry_entry_id = device_detail->get_registry_entry_id()) {
          auto queue = find_or_create_queue(*registry_entry_id);
          auto time_stamp = queued_event.get_event_time_stamp().get_time_stamp();
          queue->update_first_grabbed_event_time_stamp(time_stamp);
        }
      }
    }
  }

  void erase_queue(registry_entry_id registry_entry_id) {
    queues_.erase(registry_entry_id);
  }

private:
  std::shared_ptr<grabbable_state_queue> find_or_create_queue(registry_entry_id registry_entry_id) {
    auto it = queues_.find(registry_entry_id);
    if (it != std::end(queues_)) {
      return it->second;
    }

    auto queue = std::make_shared<grabbable_state_queue>();
    queues_[registry_entry_id] = queue;
    return queue;
  }

  std::unordered_map<registry_entry_id, std::shared_ptr<grabbable_state_queue>> queues_;
};
} // namespace krbn
