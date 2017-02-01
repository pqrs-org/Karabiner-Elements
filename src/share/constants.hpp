#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include <cstdlib>
#include <string>
#include <thread>

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

  static const char* get_user_configuration_directory(void) {
    static std::mutex mutex;
    static bool once = false;
    static std::string directory;

    std::lock_guard<std::mutex> guard(mutex);

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

    if (directory.empty()) {
      return nullptr;
    } else {
      return directory.c_str();
    }
  }

  static const char* get_user_data_directory(void) {
    static std::mutex mutex;
    static bool once = false;
    static std::string directory;

    std::lock_guard<std::mutex> guard(mutex);

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

    if (directory.empty()) {
      return nullptr;
    } else {
      return directory.c_str();
    }
  }

  static const char* get_core_configuration_file_path(void) {
    static std::mutex mutex;
    static bool once = false;
    static std::string file_path;

    std::lock_guard<std::mutex> guard(mutex);

    if (!once) {
      once = true;
      if (auto p = get_user_configuration_directory()) {
        file_path = p;
        file_path += "/karabiner.json";
      }
    }

    if (file_path.empty()) {
      return nullptr;
    } else {
      return file_path.c_str();
    }
  }

  static const std::string& get_user_log_directory(void) {
    static std::mutex mutex;
    static bool once = false;
    static std::string directory;

    std::lock_guard<std::mutex> guard(mutex);

    if (!once) {
      once = true;
      if (auto p = get_user_data_directory()) {
        directory = p;
        directory += "/log";
      }
    }

    return directory;
  }

  static const std::string& get_user_pid_directory(void) {
    static std::mutex mutex;
    static bool once = false;
    static std::string directory;

    std::lock_guard<std::mutex> guard(mutex);

    if (!once) {
      once = true;
      if (auto p = get_user_data_directory()) {
        directory = p;
        directory += "/pid";
      }
    }

    return directory;
  }

  static CFStringRef get_distributed_notification_observed_object(void) {
    return CFSTR("org.pqrs.karabiner");
  }

  static CFStringRef get_distributed_notification_grabber_is_launched(void) {
    return CFSTR("grabber_is_launched");
  }
};
