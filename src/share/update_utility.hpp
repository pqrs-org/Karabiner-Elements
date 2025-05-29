#pragma once

#include "logger.hpp"
#include "monitor/configuration_monitor.hpp"
#include <cstdlib>
#include <pqrs/osx/workspace.hpp>
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

private:
  static void launch_updater(const std::string& argument) {
    system("/usr/bin/killall Karabiner-Updater");

    pqrs::osx::workspace::open_application_by_file_path(
        "/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-Updater.app",
        pqrs::osx::workspace::open_configuration{
            .adds_to_recent_items = false,
            .arguments = {argument}});
  }
};
} // namespace krbn
