#pragma once

// `krbn::kextd::components_manager` can be used safely in a multi-threaded environment.

#include "components_manager_killer.hpp"
#include "kext_loader.hpp"
#include "monitor/version_monitor.hpp"

namespace krbn {
namespace kextd {
class components_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  components_manager(const components_manager&) = delete;

  components_manager(void) : dispatcher_client() {
    //
    // version_monitor_
    //

    version_monitor_ = std::make_shared<krbn::version_monitor>(krbn::constants::get_version_file_path());

    version_monitor_->changed.connect([](auto&& version) {
      if (auto killer = components_manager_killer::get_shared_components_manager_killer()) {
        killer->async_kill();
      }
    });

    //
    // kext_loader_
    //

    kext_loader_ = std::make_unique<kext_loader>(version_monitor_);
  }

  virtual ~components_manager(void) {
    detach_from_dispatcher([this] {
      kext_loader_ = nullptr;
      version_monitor_ = nullptr;
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      if (version_monitor_) {
        version_monitor_->async_start();
      }

      if (kext_loader_) {
        kext_loader_->async_start();
      }
    });
  }

private:
  std::shared_ptr<version_monitor> version_monitor_;
  std::unique_ptr<kext_loader> kext_loader_;
};
} // namespace kextd
} // namespace krbn
