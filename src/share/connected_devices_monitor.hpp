#pragma once

// `krbn::connected_devices_monitor` can be used safely in a multi-threaded environment.

#include "connected_devices.hpp"
#include "file_monitor.hpp"
#include "logger.hpp"

namespace krbn {
class connected_devices_monitor final {
public:
  // Signals

  boost::signals2::signal<void(std::weak_ptr<const connected_devices>)> connected_devices_updated;

  // Methods

  connected_devices_monitor(const std::string& devices_json_file_path) {
    std::vector<std::string> targets = {
        devices_json_file_path,
    };

    file_monitor_ = std::make_unique<file_monitor>(targets);

    file_monitor_->file_changed.connect([this](auto&& changed_file_path,
                                               auto&& file_body) {
      if (filesystem::exists(changed_file_path)) {
        logger::get_logger().info("Load {0}...", changed_file_path);
      }

      auto c = std::make_shared<connected_devices>(changed_file_path);

      if (connected_devices_ && !c->is_loaded()) {
        return;
      }

      {
        std::lock_guard<std::mutex> lock(connected_devices_mutex_);

        connected_devices_ = c;
      }

      logger::get_logger().info("connected_devices are updated.");

      connected_devices_updated(c);
    });
  }

  ~connected_devices_monitor(void) {
    file_monitor_ = nullptr;
  }

  void async_start() {
    file_monitor_->async_start();
  }

  std::shared_ptr<connected_devices> get_connected_devices(void) const {
    std::lock_guard<std::mutex> lock(connected_devices_mutex_);

    return connected_devices_;
  }

  std::shared_ptr<cf_utility::run_loop_thread> get_run_loop_thread(void) const {
    return file_monitor_->get_run_loop_thread();
  }

private:
  std::unique_ptr<file_monitor> file_monitor_;

  std::shared_ptr<connected_devices> connected_devices_;
  mutable std::mutex connected_devices_mutex_;
};
} // namespace krbn
