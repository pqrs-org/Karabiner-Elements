#pragma once

#include <unistd.h>

#include "constants.hpp"
#include "local_datagram_client.hpp"
#include "userspace_defs.h"

class grabber_client final {
public:
  grabber_client(void) : client_(constants::get_grabber_socket_file_path()) {}

  void connect(void) {
    krbn_operation_type_connect data;
    data.console_user_server_pid = getpid();
    client_.send_to(reinterpret_cast<uint8_t*>(&data), sizeof(data));
  }

private:
  local_datagram_client client_;
};
