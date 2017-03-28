#pragma once

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

private:
  static void launch_updater(const std::string& argument) {
    auto command = std::string("open '/Library/Application Support/org.pqrs/Karabiner-Elements/updater/Karabiner-Elements.app' --args ") + argument;
    system(command.c_str());
  }
};
} // namespace krbn
