#pragma once

#include "logger.hpp"
#include "monitor/configuration_monitor.hpp"
#include <cstdlib>
#include <string>

namespace krbn {
class update_utility final {
public:
  static void check_for_updates_in_background(void) {
    launch_updater("checkForUpdatesInBackground");
  }

private:
  static void launch_updater(const std::string& argument) {
    auto command = std::string("open -n '/Applications/Karabiner-Elements.app' --args ") + argument;
    system(command.c_str());
  }
};
} // namespace krbn
