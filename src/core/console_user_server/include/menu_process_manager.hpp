#pragma once

#include "application_launcher.hpp"
#include "boost_utility.hpp"
#include "configuration_monitor.hpp"

namespace krbn {
class menu_process_manager final {
public:
  menu_process_manager(const menu_process_manager&) = delete;

  menu_process_manager(std::shared_ptr<configuration_monitor> configuration_monitor) {
    // core_configuration_updated

    {
      auto c = configuration_monitor->core_configuration_updated.connect([](auto&& core_configuration) {
        if (core_configuration->get_global_configuration().get_show_in_menu_bar() ||
            core_configuration->get_global_configuration().get_show_profile_name_in_menu_bar()) {
          application_launcher::launch_menu();
        } else {
          application_launcher::kill_menu();
        }
      });

      connections_.push_back(std::make_unique<boost::signals2::scoped_connection>(c));
    }
  }

private:
  std::vector<std::unique_ptr<boost::signals2::scoped_connection>> connections_;
};
} // namespace krbn
