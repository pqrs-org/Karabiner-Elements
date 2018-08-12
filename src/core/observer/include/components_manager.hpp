#pragma once

#include "console_user_id_monitor.hpp"
#include "constants.hpp"
#include "device_observer.hpp"
#include "gcd_utility.hpp"
#include "grabber_client.hpp"
#include "version_monitor_utility.hpp"

namespace krbn {
class components_manager final {
public:
  components_manager(const components_manager&) = delete;

  components_manager(void) {
    version_monitor_ = version_monitor_utility::make_version_monitor_stops_main_run_loop_when_version_changed();

    start_grabber_client();
  }

  ~components_manager(void) {
    stop_grabber_client();
    stop_device_observer();
  }

private:
  void start_grabber_client(void) {
    std::lock_guard<std::mutex> lock(grabber_client_mutex_);

    if (grabber_client_) {
      return;
    }

    grabber_client_ = std::make_shared<grabber_client>();

    grabber_client_->connected.connect([this] {
      version_monitor_->manual_check();

      start_device_observer();
    });

    grabber_client_->connect_failed.connect([this](auto&& error_code) {
      version_monitor_->manual_check();

      stop_device_observer();
    });

    grabber_client_->closed.connect([this] {
      version_monitor_->manual_check();

      stop_device_observer();
    });

    grabber_client_->start();
  }

  void stop_grabber_client(void) {
    std::lock_guard<std::mutex> lock(grabber_client_mutex_);

    if (!grabber_client_) {
      return;
    }

    grabber_client_ = nullptr;
  }

  void start_device_observer(void) {
    std::lock_guard<std::mutex> lock(device_observer_mutex_);

    if (device_observer_) {
      return;
    }

    device_observer_ = std::make_shared<device_observer>(grabber_client_);
  }

  void stop_device_observer(void) {
    std::lock_guard<std::mutex> lock(device_observer_mutex_);

    if (!device_observer_) {
      return;
    }

    device_observer_ = nullptr;
  }

  std::shared_ptr<version_monitor> version_monitor_;

  std::shared_ptr<grabber_client> grabber_client_;
  std::mutex grabber_client_mutex_;

  std::shared_ptr<device_observer> device_observer_;
  std::mutex device_observer_mutex_;
};
} // namespace krbn
