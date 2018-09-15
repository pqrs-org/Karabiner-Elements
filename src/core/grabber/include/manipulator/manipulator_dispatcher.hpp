#pragma once

// `krbn::manipulator::manipulator_dispatcher` can be used safely in a multi-threaded environment.

#include "manipulator_object_id.hpp"
#include "thread_utility.hpp"
#include <unordered_set>

namespace krbn {
namespace manipulator {
class manipulator_dispatcher final {
public:
  manipulator_dispatcher(void) {
    dispatcher_ = std::make_unique<thread_utility::dispatcher>();
  }

  ~manipulator_dispatcher(void) {
    dispatcher_->enqueue([this] {
      if (!manipulator_object_ids_.empty()) {
        logger::get_logger().error("manipulator_dispatcher::manipulator_object_ids_ is not empty in ~manipulator_dispatcher.");
      }
    });

    dispatcher_->terminate();
    dispatcher_ = nullptr;
  }

  void async_attach(manipulator_object_id id) {
    dispatcher_->enqueue([this, id] {
      manipulator_object_ids_.insert(id);
    });
  }

  void detach(manipulator_object_id id) {
    if (dispatcher_->is_worker_thread()) {
      manipulator_object_ids_.erase(id);

    } else {
      thread_utility::wait wait;

      dispatcher_->enqueue([this, id, &wait] {
        detach(id);

        wait.notify();
      });

      wait.wait_notice();
    }
  }

  void enqueue(manipulator_object_id id,
               const std::function<void(void)>& function) {
    dispatcher_->enqueue([this, id, function] {
      auto it = manipulator_object_ids_.find(id);
      if (it == std::end(manipulator_object_ids_)) {
        return;
      }

      function();
    });
  }

private:
  std::unique_ptr<thread_utility::dispatcher> dispatcher_;
  std::unordered_set<manipulator_object_id> manipulator_object_ids_;
};
} // namespace manipulator
} // namespace krbn
