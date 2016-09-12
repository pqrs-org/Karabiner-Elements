#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include <cstdlib>
#include <string>
#include <thread>

class constants final {
public:
  static const char* get_socket_directory(void) {
    return "/Library/Application Support/org.pqrs/tmp";
  }

  static const char* get_grabber_socket_file_path(void) {
    return "/Library/Application Support/org.pqrs/tmp/karabiner_grabber_receiver";
  }

  static const char* get_console_user_socket_directory(void) {
    return "/Library/Application Support/org.pqrs/tmp/karabiner_console_user";
  }

  static const char* get_console_user_socket_file_path(void) {
    return "/Library/Application Support/org.pqrs/tmp/karabiner_console_user/receiver";
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

  static CFStringRef get_distributed_notification_observed_object(void) {
    return CFSTR("org.pqrs.karabiner");
  }

  static CFStringRef get_distributed_notification_grabber_is_launched(void) {
    return CFSTR("grabber_is_launched");
  }

  static CFStringRef get_distributed_notification_console_user_socket_directory_is_ready(void) {
    return CFSTR("console_user_socket_directory_is_ready");
  }
};
