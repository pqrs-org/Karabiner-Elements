#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <pqrs/osx/launchctl.hpp>
#include <spdlog/fmt/fmt.h>
#include <string>
#include <string_view>
#include <thread>

namespace krbn {
class constants final {
public:
  static const char* get_version_file_path(void) {
    return "/Library/Application Support/org.pqrs/Karabiner-Elements/version";
  }

  static const char* get_tmp_directory(void) {
    return "/Library/Application Support/org.pqrs/tmp";
  }

  static std::string get_pid_directory(void) {
    return "/Library/Application Support/org.pqrs/tmp/pid";
  }

  static std::string get_rootonly_directory(void) {
    return "/Library/Application Support/org.pqrs/tmp/rootonly";
  }

  static std::filesystem::path get_system_user_directory(void) {
    return "/Library/Application Support/org.pqrs/tmp/user";
  }

  static std::filesystem::path get_system_user_directory(uid_t uid) {
    return fmt::format("{0}/{1}", get_system_user_directory().string(), uid);
  }

  static const char* get_observer_state_json_file_path(void) {
    return "/Library/Application Support/org.pqrs/tmp/karabiner_observer_state.json";
  }

  static const char* get_grabber_state_json_file_path(void) {
    return "/Library/Application Support/org.pqrs/tmp/karabiner_grabber_state.json";
  }

  static const char* get_grabber_socket_file_path(void) {
    return "/Library/Application Support/org.pqrs/tmp/karabiner_grabber_receiver";
  }

  static std::string get_grabber_session_monitor_receiver_socket_file_path(void) {
    return get_rootonly_directory() + "/karabiner_grabber_session_monitor_receiver";
  }

  static const char* get_devices_json_file_path(void) {
    return "/Library/Application Support/org.pqrs/tmp/karabiner_grabber_devices.json";
  }

  static const char* get_device_details_json_file_path(void) {
    return "/Library/Application Support/org.pqrs/tmp/karabiner_grabber_device_details.json";
  }

  static const char* get_manipulator_environment_json_file_path(void) {
    return "/Library/Application Support/org.pqrs/tmp/karabiner_grabber_manipulator_environment.json";
  }

  static const char* get_notification_message_file_path(void) {
    return "/Library/Application Support/org.pqrs/tmp/karabiner_notification_message.json";
  }

  static std::string get_session_monitor_receiver_socket_file_path(uid_t uid) {
    return fmt::format("{0}/karabiner_session_monitor_receiver.{1}", get_rootonly_directory(), uid);
  }

  static const char* get_system_configuration_directory(void) {
    return "/Library/Application Support/org.pqrs/config";
  }

  static const char* get_system_core_configuration_file_path(void) {
    return "/Library/Application Support/org.pqrs/config/karabiner.json";
  }

  static const std::string& get_user_configuration_directory(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static bool once = false;
    static std::string directory;

    if (!once) {
      once = true;
      if (auto xdg_config_home = std::getenv("XDG_CONFIG_HOME")) {
        directory = xdg_config_home;
      } else {
        if (auto home = std::getenv("HOME")) {
          directory = home;
          directory += "/.config";
        }
      }

      if (!directory.empty()) {
        directory += "/karabiner";
      }
    }

    return directory;
  }

  static const std::string& get_user_data_directory(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static bool once = false;
    static std::string directory;

    if (!once) {
      once = true;
      if (auto xdg_config_home = std::getenv("XDG_DATA_HOME")) {
        directory = xdg_config_home;
      } else {
        if (auto home = std::getenv("HOME")) {
          directory = home;
          directory += "/.local/share";
        }
      }

      if (!directory.empty()) {
        directory += "/karabiner";
      }
    }

    return directory;
  }

  static const std::string& get_user_core_configuration_file_path(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static bool once = false;
    static std::string file_path;

    if (!once) {
      once = true;
      auto directory = get_user_configuration_directory();
      if (!directory.empty()) {
        file_path = directory + "/karabiner.json";
      }
    }

    return file_path;
  }

  static const std::string& get_user_core_configuration_automatic_backups_directory(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static bool once = false;
    static std::string directory;

    if (!once) {
      once = true;
      auto d = get_user_configuration_directory();
      if (!d.empty()) {
        directory = d + "/automatic_backups";
      }
    }

    return directory;
  }

  static const std::string& get_user_complex_modifications_assets_directory(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static bool once = false;
    static std::string directory;

    if (!once) {
      once = true;
      auto d = get_user_configuration_directory();
      if (!d.empty()) {
        directory = d + "/assets/complex_modifications";
      }
    }

    return directory;
  }

  static const std::string& get_user_log_directory(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static bool once = false;
    static std::string directory;

    if (!once) {
      once = true;
      auto d = get_user_data_directory();
      if (!d.empty()) {
        directory = d + "/log";
      }
    }

    return directory;
  }

  static const std::string& get_user_pid_directory(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static bool once = false;
    static std::string directory;

    if (!once) {
      once = true;
      auto d = get_user_data_directory();
      if (!d.empty()) {
        directory = d + "/pid";
      }
    }

    return directory;
  }

  static const char* get_distributed_notification_observed_object(void) {
    return "org.pqrs.karabiner";
  }

  static const char* get_distributed_notification_console_user_server_is_disabled(void) {
    return "console_user_server_is_disabled";
  }

  static const size_t get_local_datagram_buffer_size(void) {
    return 32 * 1024;
  }

  static pqrs::osx::launchctl::service_name get_grabber_daemon_launchctl_service_name(void) {
    return pqrs::osx::launchctl::service_name("org.pqrs.karabiner.karabiner_grabber");
  }

  static pqrs::osx::launchctl::service_path get_grabber_daemon_launchctl_service_path(void) {
    return pqrs::osx::launchctl::service_path("/Library/LaunchDaemons/org.pqrs.karabiner.karabiner_grabber.plist");
  }

  static pqrs::osx::launchctl::service_name get_observer_agent_launchctl_service_name(void) {
    return pqrs::osx::launchctl::service_name("org.pqrs.karabiner.agent.karabiner_observer");
  }

  static pqrs::osx::launchctl::service_path get_observer_agent_launchctl_service_path(void) {
    return pqrs::osx::launchctl::service_path("/Library/LaunchAgents/org.pqrs.karabiner.agent.karabiner_observer.plist");
  }

  static pqrs::osx::launchctl::service_name get_grabber_agent_launchctl_service_name(void) {
    return pqrs::osx::launchctl::service_name("org.pqrs.karabiner.agent.karabiner_grabber");
  }

  static pqrs::osx::launchctl::service_path get_grabber_agent_launchctl_service_path(void) {
    return pqrs::osx::launchctl::service_path("/Library/LaunchAgents/org.pqrs.karabiner.agent.karabiner_grabber.plist");
  }

  static pqrs::osx::launchctl::service_name get_session_monitor_launchctl_service_name(void) {
    return pqrs::osx::launchctl::service_name("org.pqrs.karabiner.karabiner_session_monitor");
  }

  static pqrs::osx::launchctl::service_path get_session_monitor_launchctl_service_path(void) {
    return pqrs::osx::launchctl::service_path("/Library/LaunchAgents/org.pqrs.karabiner.karabiner_session_monitor.plist");
  }

  static pqrs::osx::launchctl::service_name get_console_user_server_launchctl_service_name(void) {
    return pqrs::osx::launchctl::service_name("org.pqrs.karabiner.karabiner_console_user_server");
  }

  static pqrs::osx::launchctl::service_path get_console_user_server_launchctl_service_path(void) {
    return pqrs::osx::launchctl::service_path("/Library/LaunchAgents/org.pqrs.karabiner.karabiner_console_user_server.plist");
  }

  static pqrs::osx::launchctl::service_name get_notification_window_launchctl_service_name(void) {
    return pqrs::osx::launchctl::service_name("org.pqrs.karabiner.NotificationWindow");
  }

  static pqrs::osx::launchctl::service_path get_notification_window_launchctl_service_path(void) {
    return pqrs::osx::launchctl::service_path("/Library/LaunchAgents/org.pqrs.karabiner.NotificationWindow.plist");
  }
};
} // namespace krbn
