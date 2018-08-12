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

    // Setup grabber_client

    auto grabber_client = grabber_client::get_shared_instance();

    connections_.push_back(
        std::make_unique<boost::signals2::scoped_connection>(
            grabber_client->connected.connect([this] {
              version_monitor_->manual_check();

              start_device_observer();
            })));

    connections_.push_back(
        std::make_unique<boost::signals2::scoped_connection>(
            grabber_client->connect_failed.connect([this](auto&& error_code) {
              version_monitor_->manual_check();

              stop_device_observer();
            })));

    connections_.push_back(
        std::make_unique<boost::signals2::scoped_connection>(
            grabber_client->closed.connect([this] {
              version_monitor_->manual_check();

              stop_device_observer();
            })));

    grabber_client->start();
  }

private:
  void start_device_observer(void) {
    std::lock_guard<std::mutex> lock(device_observer_mutex_);

    if (!device_observer_) {
      device_observer_ = std::make_unique<device_observer>();
      logger::get_logger().info("device_observer is created.");
    }
  }

  void stop_device_observer(void) {
    std::lock_guard<std::mutex> lock(device_observer_mutex_);

    if (device_observer_) {
      device_observer_ = nullptr;
      logger::get_logger().info("device_observer is destroyed.");
    }
  }

  std::shared_ptr<version_monitor> version_monitor_;
  std::vector<std::unique_ptr<boost::signals2::scoped_connection>> connections_;

  std::unique_ptr<device_observer> device_observer_;
  std::mutex device_observer_mutex_;
};
} // namespace krbn
