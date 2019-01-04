#pragma once

// `krbn::components_manager` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "device_observer.hpp"
#include "grabber_client.hpp"
#include "monitor/version_monitor_utility.hpp"
#include <pqrs/dispatcher.hpp>

namespace krbn {
class components_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  components_manager(const components_manager&) = delete;

  components_manager(void) : dispatcher_client() {
    // version_monitor_

    version_monitor_ = version_monitor_utility::make_version_monitor_stops_main_run_loop_when_version_changed();

    // grabber_client_

    grabber_client_ = std::make_shared<grabber_client>();

    grabber_client_->connected.connect([this] {
      if (version_monitor_) {
        version_monitor_->async_manual_check();
      }

      if (device_observer_) {
        device_observer_->async_post_all_states_to_grabber();
      }
    });

    grabber_client_->connect_failed.connect([this](auto&& error_code) {
      if (version_monitor_) {
        version_monitor_->async_manual_check();
      }
    });

    grabber_client_->closed.connect([this] {
      if (version_monitor_) {
        version_monitor_->async_manual_check();
      }
    });

    // device_observer_

    device_observer_ = std::make_shared<device_observer>(grabber_client_);

    // start

    grabber_client_->async_start();
  }

  virtual ~components_manager(void) {
    detach_from_dispatcher([this] {
      device_observer_ = nullptr;

      grabber_client_ = nullptr;

      version_monitor_ = nullptr;
    });
  }

private:
  std::shared_ptr<version_monitor> version_monitor_;
  std::shared_ptr<grabber_client> grabber_client_;
  std::shared_ptr<device_observer> device_observer_;
};
} // namespace krbn
