#pragma once

namespace krbn {
namespace services_utility {

static constexpr const char* daemons_path = "/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-Elements Privileged Daemons.app/Contents/MacOS/Karabiner-Elements Privileged Daemons";
static constexpr const char* agents_path = "/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-Elements Non-Privileged Agents.app/Contents/MacOS/Karabiner-Elements Non-Privileged Agents";

//
// core_daemons
//

inline void register_core_daemons(void) {
  system(fmt::format("'{0}' register-core-daemons",
                     daemons_path)
             .c_str());
}

//
// core_agents
//

inline void register_core_agents(void) {
  system(fmt::format("'{0}' register-core-agents",
                     agents_path)
             .c_str());
}

inline void unregister_core_agents(void) {
  system(fmt::format("'{0}' unregister-core-agents",
                     agents_path)
             .c_str());
}

//
// console_user_server_agent
//

inline void restart_console_user_server_agent(void) {
  auto domain_target = pqrs::osx::launchctl::make_gui_domain_target();
  auto service_name = pqrs::osx::launchctl::service_name("org.pqrs.service.agent.karabiner_console_user_server");

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
                     agents_path)
             .c_str());
}

inline void unregister_menu_agent(void) {
  system(fmt::format("'{0}' unregister-menu-agent",
                     agents_path)
             .c_str());
}

//
// multitouch_extension_agent
//

inline void register_multitouch_extension_agent(void) {
  system(fmt::format("'{0}' register-multitouch-extension-agent",
                     agents_path)
             .c_str());
}

inline void unregister_multitouch_extension_agent(void) {
  system(fmt::format("'{0}' unregister-multitouch-extension-agent",
                     agents_path)
             .c_str());
}

//
// notification_window_agent
//

inline void register_notification_window_agent(void) {
  system(fmt::format("'{0}' register-notification-window-agent",
                     agents_path)
             .c_str());
}

inline void unregister_notification_window_agent(void) {
  system(fmt::format("'{0}' unregister-notification-window-agent",
                     agents_path)
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

inline void unregister_all_agents(void) {
  unregister_core_agents();
  unregister_multitouch_extension_agent();
  unregister_notification_window_agent();

  // `unregister_all_agents` might be called within Karabiner-Menu.app.
  // In that case, `unregister_menu_agent` will terminate itself,
  // so it needs to be called after unregistering other services.
  unregister_menu_agent();
}

inline bool core_daemons_enabled(void) {
  auto exit_code = system(fmt::format("'{0}' core-daemons-enabled",
                                      daemons_path)
                              .c_str());
  return exit_code == 0;
}

inline bool core_agents_enabled(void) {
  auto exit_code = system(fmt::format("'{0}' core-agents-enabled",
                                      agents_path)
                              .c_str());
  return exit_code == 0;
}

inline bool daemon_running(const std::string& service_name) {
  auto pid = pqrs::osx::launchctl::find_pid(pqrs::osx::launchctl::make_system_domain_target(),
                                            pqrs::osx::launchctl::service_name(service_name));
  return pid != std::nullopt;
}

inline bool agent_running(const std::string& service_name) {
  auto pid = pqrs::osx::launchctl::find_pid(pqrs::osx::launchctl::make_gui_domain_target(),
                                            pqrs::osx::launchctl::service_name(service_name));
  return pid != std::nullopt;
}

inline bool core_daemons_running(void) {
  auto exit_code = system(fmt::format("'{0}' running",
                                      daemons_path)
                              .c_str());
  return exit_code == 0;
}

inline bool core_agents_running(void) {
  auto exit_code = system(fmt::format("'{0}' running",
                                      agents_path)
                              .c_str());
  return exit_code == 0;
}

} // namespace services_utility
} // namespace krbn
