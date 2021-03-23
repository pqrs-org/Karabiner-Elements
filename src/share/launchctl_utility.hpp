#pragma once

#include "apple_notification_center.hpp"
#include "constants.hpp"
#include <cstdlib>
#include <sstream>
#include <string>

namespace krbn {
namespace launchctl_utility {
inline void bootout_grabber_daemon(void) {
  auto domain_target = pqrs::osx::launchctl::make_system_domain_target();
  auto service_path = constants::get_grabber_daemon_launchctl_service_path();

  pqrs::osx::launchctl::bootout(domain_target,
                                service_path);
}

inline void manage_observer_agent(void) {
  auto domain_target = pqrs::osx::launchctl::make_gui_domain_target();
  auto service_name = constants::get_observer_agent_launchctl_service_name();
  auto service_path = constants::get_observer_agent_launchctl_service_path();

  pqrs::osx::launchctl::enable(domain_target,
                               service_name,
                               service_path);
  pqrs::osx::launchctl::kickstart(domain_target,
                                  service_name);
}

inline void manage_grabber_agent(void) {
  auto domain_target = pqrs::osx::launchctl::make_gui_domain_target();
  auto service_name = constants::get_grabber_agent_launchctl_service_name();
  auto service_path = constants::get_grabber_agent_launchctl_service_path();

  pqrs::osx::launchctl::enable(domain_target,
                               service_name,
                               service_path);
  pqrs::osx::launchctl::kickstart(domain_target,
                                  service_name);
}

inline void manage_session_monitor(void) {
  auto domain_target = pqrs::osx::launchctl::make_gui_domain_target();
  auto service_name = constants::get_session_monitor_launchctl_service_name();
  auto service_path = constants::get_session_monitor_launchctl_service_path();

  pqrs::osx::launchctl::enable(domain_target,
                               service_name,
                               service_path);
  pqrs::osx::launchctl::kickstart(domain_target,
                                  service_name);
}

inline void manage_console_user_server(bool load) {
  auto domain_target = pqrs::osx::launchctl::make_gui_domain_target();
  auto service_name = constants::get_console_user_server_launchctl_service_name();
  auto service_path = constants::get_console_user_server_launchctl_service_path();

  if (load) {
    pqrs::osx::launchctl::enable(domain_target,
                                 service_name,
                                 service_path);
    pqrs::osx::launchctl::kickstart(domain_target,
                                    service_name);
  } else {
    pqrs::osx::launchctl::disable(domain_target,
                                  service_name,
                                  service_path);

    apple_notification_center::post_distributed_notification(constants::get_distributed_notification_console_user_server_is_disabled());
  }
}

inline void restart_console_user_server(void) {
  auto domain_target = pqrs::osx::launchctl::make_gui_domain_target();
  auto service_name = constants::get_console_user_server_launchctl_service_name();

  auto flags = pqrs::osx::launchctl::kickstart_flags::kill |
               pqrs::osx::launchctl::kickstart_flags::background;
  pqrs::osx::launchctl::kickstart(domain_target,
                                  service_name,
                                  flags);
}

inline void bootout_console_user_server(void) {
  auto domain_target = pqrs::osx::launchctl::make_gui_domain_target();
  auto service_path = constants::get_console_user_server_launchctl_service_path();

  pqrs::osx::launchctl::bootout(domain_target,
                                service_path);
}

inline void manage_notification_window(bool load) {
  auto domain_target = pqrs::osx::launchctl::make_gui_domain_target();
  auto service_name = constants::get_notification_window_launchctl_service_name();
  auto service_path = constants::get_notification_window_launchctl_service_path();

  if (load) {
    pqrs::osx::launchctl::enable(domain_target,
                                 service_name,
                                 service_path);
    pqrs::osx::launchctl::kickstart(domain_target,
                                    service_name);
  } else {
    pqrs::osx::launchctl::disable(domain_target,
                                  service_name,
                                  service_path);
  }
}

inline void restart_notification_window(void) {
  auto domain_target = pqrs::osx::launchctl::make_gui_domain_target();
  auto service_name = constants::get_notification_window_launchctl_service_name();

  auto flags = pqrs::osx::launchctl::kickstart_flags::kill |
               pqrs::osx::launchctl::kickstart_flags::background;
  pqrs::osx::launchctl::kickstart(domain_target,
                                  service_name,
                                  flags);
}
} // namespace launchctl_utility
} // namespace krbn
