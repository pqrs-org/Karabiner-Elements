#pragma once

#include <cstdlib>
#include <string>
#include <sstream>

class launchctl_utility final {
public:
  static void manage_console_user_server(bool load) {
    uid_t uid = getuid();
    auto domain_target = (std::stringstream() << "gui/" << uid).str();
    auto service_target = (std::stringstream() << "gui/" << uid << "/org.pqrs.karabiner.karabiner_console_user_server").str();
    auto service_path = "/Library/LaunchAgents/org.pqrs.karabiner.karabiner_console_user_server.plist";

    if (load) {
      // If service_path is already bootstrapped and disabled, launchctl bootstrap will fail until it is enabled again.
      // So we should enable it first, and then bootstrap and enable it.

      {
        auto command = std::string("/bin/launchctl enable ") + service_target;
        system(command.c_str());
      }
      {
        auto command = std::string("/bin/launchctl bootstrap ") + domain_target + " " + service_path;
        system(command.c_str());
      }
      {
        auto command = std::string("/bin/launchctl enable ") + service_target;
        system(command.c_str());
      }

    } else {
      {
        auto command = std::string("/bin/launchctl bootout ") + domain_target + " " + service_path;
        system(command.c_str());
      }
      {
        auto command = std::string("/bin/launchctl disable ") + service_target;
        system(command.c_str());
      }
    }
  }
};
