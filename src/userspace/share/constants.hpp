#pragma once

class constants final {
public:
  static const char* get_grabber_socket_file_path() {
    return "/Library/Application Support/org.pqrs/Karabiner/tmp/grabber_receiver";
  }

  static const char* get_console_user_socket_directory() {
    return "/Library/Application Support/org.pqrs/Karabiner/tmp/console_user";
  }

  static const char* get_console_user_socket_file_path() {
    return "/Library/Application Support/org.pqrs/Karabiner/tmp/console_user/receiver";
  }
};
