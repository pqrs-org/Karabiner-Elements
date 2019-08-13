#pragma once

#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>

namespace krbn {
class components_manager_killer final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(void)> kill_called;

  // Methods

  components_manager_killer(void) : dispatcher_client() {
  }

  virtual ~components_manager_killer(void) {
    detach_from_dispatcher();
  }

  void async_kill(void) {
    enqueue_to_dispatcher([this] {
      kill_called();
    });
  }

  static void initialize_shared_components_manager_killer(void) {
    instance_ = std::make_shared<components_manager_killer>();
  }

  static void terminate_shared_components_manager_killer(void) {
    instance_ = nullptr;
  }

  static std::shared_ptr<components_manager_killer> get_shared_components_manager_killer(void) {
    return instance_;
  }

private:
  inline static std::shared_ptr<components_manager_killer> instance_;
};
} // namespace krbn
