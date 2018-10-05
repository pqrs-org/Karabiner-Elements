#pragma once

// `krbn::menu_process_manager` can be used safely in a multi-threaded environment.

#include "application_launcher.hpp"
#include "boost_utility.hpp"
#include "dispatcher.hpp"
#include "monitor/configuration_monitor.hpp"

namespace krbn {
class menu_process_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  menu_process_manager(const menu_process_manager&) = delete;

  menu_process_manager(std::weak_ptr<configuration_monitor> weak_configuration_monitor) : dispatcher_client(),
                                                                                          weak_configuration_monitor_(weak_configuration_monitor) {
    if (auto configuration_monitor = weak_configuration_monitor_.lock()) {
      // core_configuration_updated
      {
        auto c = configuration_monitor->core_configuration_updated.connect([](auto&& weak_core_configuration) {
          if (auto core_configuration = weak_core_configuration.lock()) {
            if (core_configuration->get_global_configuration().get_show_in_menu_bar() ||
                core_configuration->get_global_configuration().get_show_profile_name_in_menu_bar()) {
              application_launcher::launch_menu();
            } else {
              application_launcher::kill_menu();
            }
          }
        });
        configuration_monitor_connections_.push_back(c);
      }
    }
  }

  ~menu_process_manager(void) {
    detach_from_dispatcher([this] {
      configuration_monitor_connections_.disconnect_all_connections();
    });
  }

private:
  std::weak_ptr<configuration_monitor> weak_configuration_monitor_;

  boost_utility::signals2_connections configuration_monitor_connections_;
};
} // namespace krbn
