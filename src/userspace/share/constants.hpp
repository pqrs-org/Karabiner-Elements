#pragma once

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
};
