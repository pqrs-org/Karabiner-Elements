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

  static const char* get_socket_directory(void) {
    return "/Library/Application Support/org.pqrs/tmp";
  }

  static const char* get_grabber_socket_file_path(void) {
    return "/Library/Application Support/org.pqrs/tmp/karabiner_grabber_receiver";
  }

  static const char* get_event_dispatcher_socket_file_path(void) {
    return "/Library/Application Support/org.pqrs/tmp/karabiner_event_dispatcher_receiver";
  }

  static const char* get_configuration_directory(void) {
    static std::mutex mutex;
    static bool once = false;
    static std::string directory;

    std::lock_guard<std::mutex> guard(mutex);

    if (!once) {
      once = true;
      if (auto x = std::getenv("XDG_CONFIG_HOME")) {
        directory = x;
        directory += "/karabiner-elements";
      } else if (auto p = std::getenv("HOME")) {
        directory = p;
        directory += "/.config/karabiner-elements";
      }
    }

    if (directory.empty()) {
      return nullptr;
    } else {
      return directory.c_str();
    }
  }

  static const char* get_logging_directory(void) {
    static std::mutex mutex;
    static bool once = false;
    static std::string directory;

    std::lock_guard<std::mutex> guard(mutex);

    if (!once) {
      once = true;
      if (auto p = std::getenv("HOME")) {
        directory = p;
        directory += "/Library/Logs/Karabiner-Elements";
      }
    }

    if (directory.empty()) {
      return nullptr;
    } else {
      return directory.c_str();
    }
  }

  static CFStringRef get_distributed_notification_observed_object(void) {
    return CFSTR("org.pqrs.karabiner");
  }

  static CFStringRef get_distributed_notification_grabber_is_launched(void) {
    return CFSTR("grabber_is_launched");
  }
};
