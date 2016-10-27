#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include <cstdlib>
#include <string>
#include <thread>

class constants final {
public:
  static const char* get_event_dispatcher_binary_file_path(void) {
    return "/Library/Application Support/org.pqrs/Karabiner-Elements/bin/karabiner_event_dispatcher";
  }

  static const char* get_tmp_directory(void) {
    return "/Library/Application Support/org.pqrs/tmp";
  }

  static const char* get_grabber_socket_file_path(void) {
    return "/Library/Application Support/org.pqrs/tmp/karabiner_grabber_receiver";
  }

  static const char* get_event_dispatcher_socket_file_path(void) {
    return "/Library/Application Support/org.pqrs/tmp/karabiner_event_dispatcher_receiver";
  }

  static const char* get_devices_json_file_path(void) {
    return "/Library/Application Support/org.pqrs/tmp/devices.json";
  }

  static const char* get_home_dot_karabiner_directory(void) {
    static std::mutex mutex;
    static bool once = false;
    static std::string directory;

    std::lock_guard<std::mutex> guard(mutex);

    if (!once) {
      once = true;
      if (auto p = std::getenv("HOME")) {
        directory = p;
        directory += "/.karabiner.d";
      }
    }

    if (directory.empty()) {
      return nullptr;
    } else {
      return directory.c_str();
    }
  }

  static const char* get_configuration_directory(void) {
    static std::mutex mutex;
    static bool once = false;
    static std::string directory;

    std::lock_guard<std::mutex> guard(mutex);

    if (!once) {
      once = true;
      if (auto p = get_home_dot_karabiner_directory()) {
        directory = p;
        directory += "/configuration";
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
      if (auto p = get_configuration_directory()) {
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

  static CFStringRef get_distributed_notification_observed_object(void) {
    return CFSTR("org.pqrs.karabiner");
  }

  static CFStringRef get_distributed_notification_grabber_is_launched(void) {
    return CFSTR("grabber_is_launched");
  }
};
