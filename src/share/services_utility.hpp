#pragma once

#include "apple_notification_center.hpp"
#include "constants.hpp"

namespace krbn {
namespace services_utility {

//
// core_daemons
//

inline void register_core_daemons(void) {
  system(fmt::format("'{0}' register-core-daemons",
                     constants::karabiner_elements_services_path)
             .c_str());
}

//
// core_agents
//

inline void register_core_agents(void) {
  system(fmt::format("'{0}' register-core-agents",
                     constants::karabiner_elements_services_path)
             .c_str());
}

inline void unregister_core_agents(void) {
  system(fmt::format("'{0}' unregister-core-agents",
                     constants::karabiner_elements_services_path)
             .c_str());
}

//
// console_user_server_agent
//

inline void restart_console_user_server_agent(void) {
  auto domain_target = pqrs::osx::launchctl::make_gui_domain_target();
  auto service_name = constants::get_console_user_server_agent_service_name();

  auto flags = pqrs::osx::launchctl::kickstart_flags::kill |
               pqrs::osx::launchctl::kickstart_flags::background;
  pqrs::osx::launchctl::kickstart(domain_target,
                                  service_name,
                                  flags);
}

//
// menu_agent
//

inline void register_menu_agent(void) {
  system(fmt::format("'{0}' register-menu-agent",
                     constants::karabiner_elements_services_path)
             .c_str());
}

inline void unregister_menu_agent(void) {
  system(fmt::format("'{0}' unregister-menu-agent",
                     constants::karabiner_elements_services_path)
             .c_str());
}

//
// notification_window_agent
//

inline void register_notification_window_agent(void) {
  system(fmt::format("'{0}' register-notification-window-agent",
                     constants::karabiner_elements_services_path)
             .c_str());
}

inline void unregister_notification_window_agent(void) {
  system(fmt::format("'{0}' unregister-notification-window-agent",
                     constants::karabiner_elements_services_path)
             .c_str());
}

//
// Old agents
//

// For old daemons, the installer can stop them, but for agents, the user needs to handle the stopping process, so the installer cannot do it.
// Additionally, simply deleting /Library/LaunchAgents will not stop launchd from processing; the old services will continue to run until bootout is explicitly called or macOS is restarted.
// Therefore, explicitly call bootout at the start of Settings and console_user_server to stop the old agents.
inline void bootout_old_agents(void) {
  auto domain_target = pqrs::osx::launchctl::make_gui_domain_target();

  for (const auto& service_name : {
           "org.pqrs.karabiner.NotificationWindow",
           "org.pqrs.karabiner.agent.karabiner_grabber",
           "org.pqrs.karabiner.karabiner_console_user_server",
           "org.pqrs.karabiner.karabiner_session_monitor",
       }) {
    pqrs::osx::launchctl::bootout(domain_target,
                                  pqrs::osx::launchctl::service_name(service_name));
  }
}

//
// Utilities
//

inline bool grabber_daemon_running(void) {
  auto pid = pqrs::osx::launchctl::get_pid(pqrs::dispatcher::extra::get_shared_dispatcher(),
                                           pqrs::osx::launchctl::make_system_domain_target(),
                                           pqrs::osx::launchctl::service_name("org.pqrs.service.daemon.karabiner_grabber"));
  return pid != std::nullopt;
}

} // namespace services_utility
} // namespace krbn
