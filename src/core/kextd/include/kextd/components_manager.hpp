#pragma once

// `krbn::kextd::components_manager` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "kext_loader.hpp"
#include "monitor/version_monitor.hpp"
#include <pqrs/dispatcher.hpp>

namespace krbn {
namespace kextd {
class components_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  components_manager(const components_manager&) = delete;

  components_manager(void) : dispatcher_client() {
    kext_loader_ = std::make_shared<kext_loader>();
  }

  virtual ~components_manager(void) {
    detach_from_dispatcher([this] {
      kext_loader_ = nullptr;
    });
  }

  void async_start(void) const {
    enqueue_to_dispatcher([this] {
      kext_loader_->async_start();
    });
  }

private:
  std::shared_ptr<kext_loader> kext_loader_;
};
} // namespace kextd
} // namespace krbn
