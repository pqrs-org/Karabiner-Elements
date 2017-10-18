#pragma once

#include "constants.hpp"
#include "filesystem.hpp"
#include "local_datagram_client.hpp"
#include "logger.hpp"
#include "session.hpp"
#include "types.hpp"
#include <sstream>
#include <unistd.h>
#include <vector>

namespace krbn {
class console_user_server_client final {
public:
  console_user_server_client(const console_user_server_client&) = delete;

  console_user_server_client(uid_t uid) {
    auto socket_file_path = make_console_user_server_socket_file_path(uid);

    // Check socket file existance
    if (!filesystem::exists(socket_file_path)) {
      throw std::runtime_error("console_user_server socket is not found");
    }

    // Check socket file permission
    if (!filesystem::is_owned(socket_file_path, uid)) {
      throw std::runtime_error("console_user_server socket owner is invalid");
    }

    client_ = std::make_unique<local_datagram_client>(socket_file_path.c_str());
  }

  void shell_command_execution(const std::string& shell_command) {
    operation_type_shell_command_execution_struct s;

    if (shell_command.length() >= sizeof(s.shell_command)) {
      logger::get_logger().error("shell_command is too long: {0}", shell_command);
      return;
    }

    strlcpy(s.shell_command,
            shell_command.c_str(),
            sizeof(s.shell_command));

    client_->send_to(reinterpret_cast<uint8_t*>(&s), sizeof(s));
  }

  void select_input_source(const input_source_selector& input_source_selector) {
    operation_type_select_input_source_struct s;

    if (auto& v = input_source_selector.get_language_string()) {
      if (v->length() >= sizeof(s.language)) {
        logger::get_logger().error("language is too long: {0}", *v);
        return;
      }

      strlcpy(s.language,
              v->c_str(),
              sizeof(s.language));
    }

    if (auto& v = input_source_selector.get_input_source_id_string()) {
      if (v->length() >= sizeof(s.input_source_id)) {
        logger::get_logger().error("input_source_id is too long: {0}", *v);
        return;
      }

      strlcpy(s.input_source_id,
              v->c_str(),
              sizeof(s.input_source_id));
    }

    if (auto& v = input_source_selector.get_input_mode_id_string()) {
      if (v->length() >= sizeof(s.input_mode_id)) {
        logger::get_logger().error("input_mode_id is too long: {0}", *v);
        return;
      }

      strlcpy(s.input_mode_id,
              v->c_str(),
              sizeof(s.input_mode_id));
    }

    client_->send_to(reinterpret_cast<uint8_t*>(&s), sizeof(s));
  }

  static std::string make_console_user_server_socket_directory(uid_t uid) {
    std::stringstream ss;
    ss << constants::get_console_user_server_socket_directory() << "/" << uid;
    return ss.str();
  }

  static std::string make_console_user_server_socket_file_path(uid_t uid) {
    return make_console_user_server_socket_directory(uid) + "/receiver";
  }

private:
  std::unique_ptr<local_datagram_client> client_;
};
} // namespace krbn
