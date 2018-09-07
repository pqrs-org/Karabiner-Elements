#pragma once

// `krbn::components_manager` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "device_observer.hpp"
#include "grabber_client.hpp"
#include "monitor/version_monitor_utility.hpp"
#include "thread_utility.hpp"

namespace krbn {
class components_manager final {
public:
  components_manager(const components_manager&) = delete;

  components_manager(void) {
    dispatcher_ = std::make_unique<thread_utility::dispatcher>();

    version_monitor_ = version_monitor_utility::make_version_monitor_stops_main_run_loop_when_version_changed();

    async_start_grabber_client();
  }

  ~components_manager(void) {
    async_stop_grabber_client();
    async_stop_device_observer();

    dispatcher_->enqueue([this] {
      version_monitor_ = nullptr;
    });

    dispatcher_->terminate();
    dispatcher_ = nullptr;
  }

private:
  void async_start_grabber_client(void) {
    dispatcher_->enqueue([this] {
      if (grabber_client_) {
        return;
      }

      grabber_client_ = std::make_shared<grabber_client>();

      grabber_client_->connected.connect([this] {
        dispatcher_->enqueue([this] {
          if (version_monitor_) {
            version_monitor_->async_manual_check();
          }

          async_start_device_observer();
        });
      });

      grabber_client_->connect_failed.connect([this](auto&& error_code) {
        dispatcher_->enqueue([this] {
          if (version_monitor_) {
            version_monitor_->async_manual_check();
          }

          async_stop_device_observer();
        });
      });

      grabber_client_->closed.connect([this] {
        dispatcher_->enqueue([this] {
          if (version_monitor_) {
            version_monitor_->async_manual_check();
          }

          async_stop_device_observer();
        });
      });

      grabber_client_->async_start();
    });
  }

  void async_stop_grabber_client(void) {
    dispatcher_->enqueue([this] {
      if (!grabber_client_) {
        return;
      }

      grabber_client_ = nullptr;
    });
  }

  void async_start_device_observer(void) {
    dispatcher_->enqueue([this] {
      if (device_observer_) {
        return;
      }

      device_observer_ = std::make_shared<device_observer>(grabber_client_);
    });
  }

  void async_stop_device_observer(void) {
    dispatcher_->enqueue([this] {
      if (!device_observer_) {
        return;
      }

      device_observer_ = nullptr;
    });
  }

  std::unique_ptr<thread_utility::dispatcher> dispatcher_;

  std::shared_ptr<version_monitor> version_monitor_;
  std::shared_ptr<grabber_client> grabber_client_;
  std::shared_ptr<device_observer> device_observer_;
};
} // namespace krbn
