#pragma once

// `krbn::observer::components_manager` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "device_observer.hpp"
#include "grabber_client.hpp"
#include "monitor/version_monitor.hpp"
#include <pqrs/dispatcher.hpp>

namespace krbn {
namespace observer {
class components_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  components_manager(const components_manager&) = delete;

  components_manager(std::weak_ptr<version_monitor> weak_version_monitor) : dispatcher_client(),
                                                                            weak_version_monitor_(weak_version_monitor) {
    // grabber_client_

    grabber_client_ = std::make_shared<grabber_client>();

    grabber_client_->connected.connect([this] {
      if (auto m = weak_version_monitor_.lock()) {
        m->async_manual_check();
      }

      if (device_observer_) {
        device_observer_->async_send_observed_devices();
        device_observer_->async_rescan();
      }
    });

    grabber_client_->connect_failed.connect([this](auto&& error_code) {
      if (auto m = weak_version_monitor_.lock()) {
        m->async_manual_check();
      }
    });

    grabber_client_->closed.connect([this] {
      if (auto m = weak_version_monitor_.lock()) {
        m->async_manual_check();
      }
    });

    // device_observer_

    device_observer_ = std::make_shared<device_observer>(grabber_client_);
  }

  virtual ~components_manager(void) {
    detach_from_dispatcher([this] {
      device_observer_ = nullptr;
      grabber_client_ = nullptr;
    });
  }

  void async_start(void) const {
    enqueue_to_dispatcher([this] {
      device_observer_->async_start();
      grabber_client_->async_start();
    });
  }

private:
  std::weak_ptr<version_monitor> weak_version_monitor_;
  std::shared_ptr<grabber_client> grabber_client_;
  std::shared_ptr<device_observer> device_observer_;
};
} // namespace observer
} // namespace krbn
