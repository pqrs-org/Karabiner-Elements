#pragma once

// `krbn::grabbable_state_queues_manager` can be used safely in a multi-threaded environment.

#include "event_queue.hpp"
#include "grabbable_state_queue.hpp"
#include <mutex>
#include <nod/nod.hpp>
#include <optional>
#include <pqrs/osx/iokit_types.hpp>
#include <unordered_map>

namespace krbn {
class grabbable_state_queues_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(device_id, std::optional<grabbable_state>)> grabbable_state_changed;

  // Methods

  grabbable_state_queues_manager(const grabbable_state_queues_manager&) = delete;

  grabbable_state_queues_manager(void) : dispatcher_client() {
  }

  virtual ~grabbable_state_queues_manager(void) {
    detach_from_dispatcher([] {
    });
  }

  std::optional<grabbable_state> find_current_grabbable_state(device_id device_id) const {
    std::lock_guard<std::mutex> lock(queues_mutex_);

    auto it = queues_.find(device_id);
    if (it != std::end(queues_)) {
      return it->second->find_current_grabbable_state();
    }

    return std::nullopt;
  }

  void clear(void) {
    std::lock_guard<std::mutex> lock(queues_mutex_);

    queues_.clear();
  }

  bool update_grabbable_state(const grabbable_state& state) {
    auto queue = find_or_create_queue(state.get_device_id());
    return queue->push_back_grabbable_state(state);
  }

  bool update_first_grabbed_event_time_stamp(const event_queue::queue& event_queue) {
    bool result = false;

    for (const auto& entry : event_queue.get_entries()) {
      auto device_id = entry.get_device_id();
      auto queue = find_or_create_queue(device_id);
      auto time_stamp = entry.get_event_time_stamp().get_time_stamp();
      if (queue->update_first_grabbed_event_time_stamp(time_stamp)) {
        logger::get_logger()->info("first grabbed event: device_id:{0} time_stamp:{1}",
                                  type_safe::get(device_id),
                                  type_safe::get(time_stamp));

        result = true;
      }
    }

    return result;
  }

  void unset_first_grabbed_event_time_stamp(device_id device_id) {
    auto queue = find_or_create_queue(device_id);
    queue->unset_first_grabbed_event_time_stamp();
  }

  void erase_queue(device_id device_id) {
    std::lock_guard<std::mutex> lock(queues_mutex_);

    queues_.erase(device_id);
  }

private:
  std::shared_ptr<grabbable_state_queue> find_or_create_queue(device_id device_id) {
    std::lock_guard<std::mutex> lock(queues_mutex_);

    auto it = queues_.find(device_id);
    if (it != std::end(queues_)) {
      return it->second;
    }

    auto queue = std::make_shared<grabbable_state_queue>();
    queue->grabbable_state_changed.connect([this, device_id](auto&& grabbable_state) {
      enqueue_to_dispatcher([this, device_id, grabbable_state] {
        grabbable_state_changed(device_id, grabbable_state);
      });
    });

    queues_[device_id] = queue;
    return queue;
  }

  std::unordered_map<device_id, std::shared_ptr<grabbable_state_queue>> queues_;
  mutable std::mutex queues_mutex_;
};
} // namespace krbn
