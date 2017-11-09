#pragma once

#include "application_launcher.hpp"
#include "configuration_monitor.hpp"
#include "constants.hpp"
#include "core_configuration.hpp"
#include "file_monitor.hpp"
#include "filesystem.hpp"
#include "logger.hpp"
#include <CoreServices/CoreServices.h>
#include <memory>

namespace krbn {
class configuration_manager final {
public:
  configuration_manager(const configuration_manager&) = delete;

  configuration_manager(void) {
    filesystem::create_directory_with_intermediate_directories(constants::get_user_configuration_directory(), 0700);

    application_launcher::kill_menu();

    configuration_monitor_ = std::make_unique<configuration_monitor>(constants::get_user_core_configuration_file_path(),
                                                                     std::bind(&configuration_manager::core_configuration_updated_callback, this, std::placeholders::_1));
  }

private:
  void core_configuration_updated_callback(std::shared_ptr<core_configuration> core_configuration) {
    if (!core_configuration) {
      return;
    }

    // ----------------------------------------
    // Launch menu
    if (core_configuration->get_global_configuration().get_show_in_menu_bar() ||
        core_configuration->get_global_configuration().get_show_profile_name_in_menu_bar()) {
      application_launcher::launch_menu();
    }
  }

  std::unique_ptr<configuration_monitor> configuration_monitor_;
};
} // namespace krbn
