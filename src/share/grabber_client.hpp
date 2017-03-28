#pragma once

#include "constants.hpp"
#include "filesystem.hpp"
#include "local_datagram_client.hpp"
#include "session.hpp"
#include "types.hpp"
#include <unistd.h>
#include <vector>

namespace krbn {
class grabber_client final {
public:
  grabber_client(const grabber_client&) = delete;

  grabber_client(void) {
    // Check socket file existance
    if (!filesystem::exists(constants::get_grabber_socket_file_path())) {
      throw std::runtime_error("grabber socket is not found");
    }

    // Check socket file permission
    if (auto current_console_user_id = session::get_current_console_user_id()) {
      if (!filesystem::is_owned(constants::get_grabber_socket_file_path(), *current_console_user_id)) {
        throw std::runtime_error("grabber socket is not writable");
      }
    } else {
      throw std::runtime_error("session::get_current_console_user_id error");
    }

    client_ = std::make_unique<local_datagram_client>(constants::get_grabber_socket_file_path());
  }

  void connect(void) {
    operation_type_connect_struct s;
    s.pid = getpid();
    client_->send_to(reinterpret_cast<uint8_t*>(&s), sizeof(s));
  }

  void system_preferences_values_updated(const system_preferences::values& values) {
    operation_type_system_preferences_values_updated_struct s;
    s.values = values;
    client_->send_to(reinterpret_cast<uint8_t*>(&s), sizeof(s));
  }

private:
  std::unique_ptr<local_datagram_client> client_;
};
} // namespace krbn
