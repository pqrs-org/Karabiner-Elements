#pragma once

#include "boost_utility.hpp"
#include "configuration_monitor.hpp"
#include "update_utility.hpp"

namespace krbn {
class updater_process_manager final {
public:
  updater_process_manager(const updater_process_manager&) = delete;

  updater_process_manager(std::weak_ptr<configuration_monitor> weak_configuration_monitor) : weak_configuration_monitor_(weak_configuration_monitor),
                                                                                             checked_(false) {
    if (auto configuration_monitor = weak_configuration_monitor_.lock()) {
      // core_configuration_updated
      {
        auto c = configuration_monitor->core_configuration_updated.connect([this](auto&& weak_core_configuration) {
          if (auto core_configuration = weak_core_configuration.lock()) {
            if (!checked_) {
              checked_ = true;

              if (core_configuration->get_global_configuration().get_check_for_updates_on_startup()) {
                logger::get_logger().info("Check for updates...");
                update_utility::check_for_updates_in_background();
              }
            }
          }
        });
        configuration_monitor_connections_.push_back(c);
      }
    }
  }

  ~updater_process_manager(void) {
    // Disconnect `configuration_monitor_connections_`.

    if (auto configuration_monitor = weak_configuration_monitor_.lock()) {
      configuration_monitor->get_run_loop_thread()->enqueue(^{
        configuration_monitor_connections_.disconnect_all_connections();
      });
    } else {
      configuration_monitor_connections_.disconnect_all_connections();
    }

    configuration_monitor_connections_.wait_disconnect_all_connections();
  }

private:
  std::weak_ptr<configuration_monitor> weak_configuration_monitor_;

  boost_utility::signals2_connections configuration_monitor_connections_;
  bool checked_;
};
} // namespace krbn
