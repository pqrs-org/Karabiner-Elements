#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include <cstdlib>
#include <string>
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

  static const char* get_grabber_socket_file_path(void) {
    return "/Library/Application Support/org.pqrs/tmp/karabiner_grabber_receiver";
  }

  static const char* get_devices_json_file_path(void) {
    return "/Library/Application Support/org.pqrs/tmp/karabiner_grabber_devices.json";
  }

  static const char* get_system_configuration_directory(void) {
    return "/Library/Application Support/org.pqrs/config";
  }

  static const char* get_system_core_configuration_file_path(void) {
    return "/Library/Application Support/org.pqrs/config/karabiner2.json";
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
        file_path = directory + "/karabiner2.json";
      }
    }

    return file_path;
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

  static const char* get_distributed_notification_grabber_is_launched(void) {
    return "grabber_is_launched";
  }

  static const char* get_distributed_notification_console_user_server_is_disabled(void) {
    return "console_user_server_is_disabled";
  }
};
} // namespace krbn
