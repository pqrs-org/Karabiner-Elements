#pragma once

#include "constants.hpp"
#include "local_datagram_client.hpp"
#include "userspace_types.hpp"
#include <unistd.h>
#include <vector>

class grabber_client final {
public:
  grabber_client(const grabber_client&) = delete;

  grabber_client(void) : client_(constants::get_grabber_socket_file_path()) {}

  void connect(void) {
    krbn::operation_type_connect_struct s;
    s.console_user_server_pid = getpid();
    client_.send_to(reinterpret_cast<uint8_t*>(&s), sizeof(s));
  }

  void clear_simple_modifications(void) {
    krbn::operation_type_clear_simple_modifications_struct s;
    client_.send_to(reinterpret_cast<uint8_t*>(&s), sizeof(s));
  }

  void add_simple_modification(krbn::key_code from_key_code, krbn::key_code to_key_code) {
    krbn::operation_type_add_simple_modification_struct s;
    s.from_key_code = from_key_code;
    s.to_key_code = to_key_code;
    client_.send_to(reinterpret_cast<uint8_t*>(&s), sizeof(s));
  }

private:
  local_datagram_client client_;
};
