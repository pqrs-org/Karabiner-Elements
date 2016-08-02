#pragma once

#include "local_datagram_server.hpp"

class grabber_observer final {
public:
  std::thread start(void) {
    const char* path = "/tmp/karabiner_observer";
    unlink(path);
    server_ = std::make_unique<local_datagram_server>(path);

    chmod(path, 0600);

    return std::thread([this] { this->worker(); });
  }

  void stop(void) {
  }

  void worker(void) {
    if (!server_) {
      return;
    }

    for (;;)
    {
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
