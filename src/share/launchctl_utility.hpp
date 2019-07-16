#pragma once

#include "apple_notification_center.hpp"
#include "constants.hpp"
#include <cstdlib>
#include <sstream>
#include <string>

namespace krbn {
namespace launchctl_utility {
inline void enable_agent(const std::string& service_name,
                         const std::string& service_path) {
  uid_t uid = geteuid();
  auto domain_target = (std::stringstream() << "gui/" << uid).str();
  auto service_target = (std::stringstream() << "gui/" << uid << "/" << service_name).str();

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
}

inline void disable_agent(const std::string& service_name,
                          const std::string& service_path) {
  uid_t uid = geteuid();
  auto domain_target = (std::stringstream() << "gui/" << uid).str();
  auto service_target = (std::stringstream() << "gui/" << uid << "/" << service_name).str();

  {
    auto command = std::string("/bin/launchctl bootout ") + domain_target + " " + service_path;
    system(command.c_str());
  }
  {
    auto command = std::string("/bin/launchctl disable ") + service_target;
    system(command.c_str());
  }
}

inline void restart_agent(const std::string& service_name) {
  uid_t uid = geteuid();
  auto service_target = (std::stringstream() << "gui/" << uid << "/" << service_name).str();

  {
    auto command = std::string("/bin/launchctl kickstart -k ") + service_target;
    system(command.c_str());
  }
}

inline void manage_observer_agent(void) {
  std::string service_name("org.pqrs.karabiner.agent.karabiner_observer");
  std::string service_path("/Library/LaunchAgents/org.pqrs.karabiner.agent.karabiner_observer.plist");

  enable_agent(service_name, service_path);
  restart_agent(service_name);
}

inline void manage_grabber_agent(void) {
  std::string service_name("org.pqrs.karabiner.agent.karabiner_grabber");
  std::string service_path("/Library/LaunchAgents/org.pqrs.karabiner.agent.karabiner_grabber.plist");

  enable_agent(service_name, service_path);
  restart_agent(service_name);
}

inline void manage_session_monitor(void) {
  enable_agent("org.pqrs.karabiner.karabiner_session_monitor",
               "/Library/LaunchAgents/org.pqrs.karabiner.karabiner_session_monitor.plist");
}

inline void manage_console_user_server(bool load) {
  std::string service_name("org.pqrs.karabiner.karabiner_console_user_server");
  std::string service_path("/Library/LaunchAgents/org.pqrs.karabiner.karabiner_console_user_server.plist");

  if (load) {
    enable_agent(service_name, service_path);
  } else {
    disable_agent(service_name, service_path);
    apple_notification_center::post_distributed_notification(constants::get_distributed_notification_console_user_server_is_disabled());
  }
}

inline void restart_console_user_server(void) {
  std::string service_name("org.pqrs.karabiner.karabiner_console_user_server");

  restart_agent(service_name);
}
} // namespace launchctl_utility
} // namespace krbn
