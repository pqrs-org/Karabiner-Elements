#pragma once

#include "constants.hpp"
#include "local_datagram_client.hpp"
#include "types.hpp"

class console_user_client final {
public:
  console_user_client(const console_user_client&) = delete;

  console_user_client(void) : client_(constants::get_console_user_socket_file_path()) {}

  void stop_key_repeat(void) {
    krbn::operation_type_stop_key_repeat_struct s;
    client_.send_to(reinterpret_cast<uint8_t*>(&s), sizeof(s));
  }

  void post_modifier_flags(krbn::key_code key_code, IOOptionBits flags) {
    krbn::operation_type_post_modifier_flags_struct s;
    s.key_code = key_code;
    s.flags = flags;
    client_.send_to(reinterpret_cast<uint8_t*>(&s), sizeof(s));
  }

  void post_key(krbn::key_code key_code, krbn::event_type event_type, IOOptionBits flags) {
    krbn::operation_type_post_key_struct s;
    s.key_code = key_code;
    s.event_type = event_type;
    s.flags = flags;
    client_.send_to(reinterpret_cast<uint8_t*>(&s), sizeof(s));
  }

private:
  local_datagram_client client_;
};
