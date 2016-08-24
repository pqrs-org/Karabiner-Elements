#pragma once

#include "constants.hpp"
#include "local_datagram_client.hpp"
#include "userspace_defs.h"
#include <unistd.h>
#include <vector>

class grabber_client final {
public:
  grabber_client(void) : client_(constants::get_grabber_socket_file_path()) {}

  void connect(void) {
    krbn_operation_type_connect_struct s;
    s.operation_type = krbn_operation_type_connect;
    s.console_user_server_pid = getpid();
    client_.send_to(reinterpret_cast<uint8_t*>(&s), sizeof(s));
  }

  void define_simple_modifications(const uint32_t* _Nullable buffer, size_t size) {
    std::vector<uint32_t> v;
    v.resize(sizeof(krbn_operation_type_define_simple_modifications_struct) / sizeof(uint32_t) + 1 + size);

    auto s = reinterpret_cast<krbn_operation_type_define_simple_modifications_struct*>(&(v[0]));
    s->operation_type = krbn_operation_type_define_simple_modifications;
    s->size = size;
    for (size_t i = 0; i < size; ++i) {
      (s->data)[i] = buffer[i];
    }

    client_.send_to(reinterpret_cast<uint8_t*>(&v[0]), v.size() * sizeof(uint32_t));
  }

private:
  local_datagram_client client_;
};
