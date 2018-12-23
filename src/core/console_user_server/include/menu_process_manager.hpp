#pragma once

// `krbn::menu_process_manager` can be used safely in a multi-threaded environment.

#include "application_launcher.hpp"
#include "monitor/configuration_monitor.hpp"
#include <pqrs/dispatcher.hpp>

namespace krbn {
class menu_process_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  menu_process_manager(const menu_process_manager&) = delete;

  menu_process_manager(std::weak_ptr<configuration_monitor> weak_configuration_monitor) : dispatcher_client(),
                                                                                          weak_configuration_monitor_(weak_configuration_monitor) {
    if (auto configuration_monitor = weak_configuration_monitor_.lock()) {
      external_signal_connections_.emplace_back(
          configuration_monitor->core_configuration_updated.connect([](auto&& weak_core_configuration) {
            if (auto core_configuration = weak_core_configuration.lock()) {
              if (core_configuration->get_global_configuration().get_show_in_menu_bar() ||
                  core_configuration->get_global_configuration().get_show_profile_name_in_menu_bar()) {
                application_launcher::launch_menu();
              } else {
                application_launcher::kill_menu();
              }
            }
          }));
    }
  }

  ~menu_process_manager(void) {
    detach_from_dispatcher([this] {
      external_signal_connections_.clear();
    });
  }

private:
  std::weak_ptr<configuration_monitor> weak_configuration_monitor_;
  std::vector<nod::scoped_connection> external_signal_connections_;
};
} // namespace krbn
