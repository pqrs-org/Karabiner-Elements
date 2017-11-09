#pragma once

#include "configuration_monitor.hpp"
#include "logger.hpp"
#include <cstdlib>
#include <string>

namespace krbn {
class update_utility final {
public:
  static void check_for_updates_in_background(void) {
    launch_updater("checkForUpdatesInBackground");
  }

  static void check_for_updates_stable_only(void) {
    launch_updater("checkForUpdatesStableOnly");
  }

  static void check_for_updates_with_beta_version(void) {
    launch_updater("checkForUpdatesWithBetaVersion");
  }

  static void check_for_updates_on_startup(void) {
    configuration_monitor cm(constants::get_user_core_configuration_file_path(),
                             [](auto&& core_configuration) {});
    if (auto core_configuration = cm.get_core_configuration()) {
      if (core_configuration->get_global_configuration().get_check_for_updates_on_startup()) {
        logger::get_logger().info("Check for updates...");
        check_for_updates_in_background();
      }
    }
  }

private:
  static void launch_updater(const std::string& argument) {
    auto command = std::string("open '/Library/Application Support/org.pqrs/Karabiner-Elements/updater/Karabiner-Elements.app' --args ") + argument;
    system(command.c_str());
  }
};
} // namespace krbn
