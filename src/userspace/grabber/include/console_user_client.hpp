#pragma once

#include "constants.hpp"
#include "local_datagram_client.hpp"
#include "userspace_defs.h"

class console_user_client final {
public:
  console_user_client(void) : client_(constants::get_console_user_socket_file_path()) {}

  void stop_key_repeat(void) {
    krbn_operation_type_stop_key_repeat data;
    data.operation_type = KRBN_OPERATION_TYPE_STOP_KEY_REPEAT;
    client_.send_to(reinterpret_cast<uint8_t*>(&data), sizeof(data));
  }

  void post_modifier_flags(IOOptionBits flags) {
    krbn_operation_type_post_modifier_flags data;
    data.operation_type = KRBN_OPERATION_TYPE_POST_MODIFIER_FLAGS;
    data.flags = flags;
    client_.send_to(reinterpret_cast<uint8_t*>(&data), sizeof(data));
  }

  void post_key(krbn_key_code key_code, krbn_event_type event_type, IOOptionBits flags) {
    krbn_operation_type_post_key data;
    data.operation_type = KRBN_OPERATION_TYPE_POST_KEY;
    data.key_code = key_code;
    data.event_type = event_type;
    data.flags = flags;
    client_.send_to(reinterpret_cast<uint8_t*>(&data), sizeof(data));
  }

private:
  local_datagram_client client_;
};
