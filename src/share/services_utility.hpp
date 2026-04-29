#pragma once

#include <pqrs/osx/launchctl.hpp>
#include <pqrs/process.hpp>

namespace krbn {
namespace services_utility {

static constexpr const char* daemons_path = "/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-Elements Privileged Daemons v2.app/Contents/MacOS/Karabiner-Elements Privileged Daemons v2";
static constexpr const char* agents_path = "/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-Elements Non-Privileged Agents v2.app/Contents/MacOS/Karabiner-Elements Non-Privileged Agents v2";

inline std::optional<int> run_command(const char* path,
                                      std::string_view argument) {
  return pqrs::process::system(fmt::format("'{0}' {1}",
                                           path,
                                           argument));
}

inline std::optional<bool> command_result_to_optional_bool(const std::optional<int>& exit_code) {
  // This method is designed to be used with the exit codes returned by
  // `enabled` and `running` in the following files:
  //
  // - src/apps/ServiceManager-Non-Privileged-Agents/src/main.swift
  // - src/apps/ServiceManager-Privileged-Daemons/src/main.swift
  //
  // During updates, these apps may be removed temporarily while console_user_server is still running.
  // In that case, `system` may return `127` (or another non-0/non-1 exit code depending on the failure mode),
  // so treat anything other than 0/1 as "unknown" rather than forcing false.
  if (exit_code == std::optional<int>(0)) {
    return true;
  }

  if (exit_code == std::optional<int>(1)) {
    return false;
  }

  return std::nullopt;
}

//
// core_daemons
//

inline void register_core_daemons(void) {
  run_command(daemons_path,
              "register-core-daemons");
}

//
// core_agents
//

inline void register_core_agents(void) {
  run_command(agents_path,
              "register-core-agents");
}

inline void unregister_core_agents(void) {
  run_command(agents_path,
              "unregister-core-agents");
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
  run_command(agents_path,
              "register-menu-agent");
}

inline void unregister_menu_agent(void) {
  run_command(agents_path,
              "unregister-menu-agent");
}

//
// multitouch_extension_agent
//

inline void register_multitouch_extension_agent(void) {
  run_command(agents_path,
              "register-multitouch-extension-agent");
}

inline void unregister_multitouch_extension_agent(void) {
  run_command(agents_path,
              "unregister-multitouch-extension-agent");
}

//
// notification_window_agent
//

inline void register_notification_window_agent(void) {
  run_command(agents_path,
              "register-notification-window-agent");
}

inline void unregister_notification_window_agent(void) {
  run_command(agents_path,
              "unregister-notification-window-agent");
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

inline std::optional<bool> core_daemons_enabled(void) {
  return command_result_to_optional_bool(run_command(daemons_path,
                                                     "core-daemons-enabled"));
}

inline std::optional<bool> core_agents_enabled(void) {
  return command_result_to_optional_bool(run_command(agents_path,
                                                     "core-agents-enabled"));
}

inline std::optional<bool> core_daemons_running(void) {
  return command_result_to_optional_bool(run_command(daemons_path,
                                                     "running"));
}

inline std::optional<bool> core_agents_running(void) {
  return command_result_to_optional_bool(run_command(agents_path,
                                                     "running"));
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

} // namespace services_utility
} // namespace krbn
