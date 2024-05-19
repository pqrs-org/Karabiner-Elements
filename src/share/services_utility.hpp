#pragma once

#include "apple_notification_center.hpp"
#include "constants.hpp"

namespace krbn {
namespace services_utility {

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

inline void restart_notification_window_agent(void) {
  auto domain_target = pqrs::osx::launchctl::make_gui_domain_target();
  auto service_name = constants::get_notification_window_agent_service_name();

  auto flags = pqrs::osx::launchctl::kickstart_flags::kill |
               pqrs::osx::launchctl::kickstart_flags::background;
  pqrs::osx::launchctl::kickstart(domain_target,
                                  service_name,
                                  flags);
}

} // namespace services_utility
} // namespace krbn
