#pragma once

#include "constants.hpp"
#include "local_datagram_client.hpp"
#include "modifier_flag_manager.hpp"
#include "userspace_defs.h"

class console_user_client final {
public:
  console_user_client(const modifier_flag_manager& modifier_flag_manager) : modifier_flag_manager_(modifier_flag_manager),
                                                                            client_(constants::get_console_user_socket_file_path()) {}

  void stop_key_repeat(void) {
    krbn_operation_type_stop_key_repeat data;
    data.operation_type = KRBN_OPERATION_TYPE_STOP_KEY_REPEAT;
    client_.send_to(reinterpret_cast<uint8_t*>(&data), sizeof(data));
  }

  void post_modifier_flags(void) {
    krbn_operation_type_post_modifier_flags data;
    data.operation_type = KRBN_OPERATION_TYPE_POST_MODIFIER_FLAGS;
    data.flags = modifier_flag_manager_.get_io_option_bits();
    client_.send_to(reinterpret_cast<uint8_t*>(&data), sizeof(data));
  }

  void post_key(krbn_key_code key_code, krbn_event_type event_type) {
    krbn_operation_type_post_key data;
    data.operation_type = KRBN_OPERATION_TYPE_POST_KEY;
    data.key_code = key_code;
    data.event_type = event_type;
    data.flags = modifier_flag_manager_.get_io_option_bits();
    client_.send_to(reinterpret_cast<uint8_t*>(&data), sizeof(data));
  }

private:
  const modifier_flag_manager& modifier_flag_manager_;
  local_datagram_client client_;
};
