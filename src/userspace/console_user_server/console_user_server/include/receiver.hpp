#pragma once

#include "constants.hpp"
#include "local_datagram_server.hpp"

class receiver final {
public:
  std::thread start(void) {
    try {
      const char* path = constants::get_console_user_socket_file_path();
      unlink(path);
      server_ = std::make_unique<local_datagram_server>(path);
      chmod(path, 0600);
    } catch (...) {
      std::cout << "default exception";
    }

    return std::thread([this] { this->worker(); });
  }

  void stop(void) {
  }

  void worker(void) {
    if (!server_) {
      return;
    }

    for (;;) {
      asio::local::datagram_protocol::endpoint sender_endpoint;
      size_t length = server_->get_socket().receive_from(asio::buffer(buffer_), sender_endpoint);
      std::cout << length << std::endl;
    }
  }

private:
  enum {
    buffer_length = 8 * 1024,
  };
  std::array<uint8_t, buffer_length> buffer_;
  std::unique_ptr<local_datagram_server> server_;
};
