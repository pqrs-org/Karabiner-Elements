#pragma once

#include "local_datagram_server.hpp"
#include "session.hpp"

class grabber_server final {
public:
  std::thread start(void) {
    const char* path = "/tmp/karabiner_grabber";
    unlink(path);
    server_ = std::make_unique<local_datagram_server>(path);

    uid_t uid;
    if (session::get_current_console_user_id(uid)) {
      chown(path, uid, 0);
    }
    chmod(path, 0600);

    return std::thread([this] { this->worker(); });
  }

  void stop(void) {
  }

  void worker(void) {
    if (!server_) {
      return;
    }

    for (;;) {
      std::cout << "receive_from" << std::endl;
      boost::asio::local::datagram_protocol::endpoint sender_endpoint;
      size_t length = server_->get_socket().receive_from(boost::asio::buffer(buffer_), sender_endpoint);
      std::cout << length << std::endl;
    }
  }

private:
  enum {
    buffer_length = 8 * 1024 * 1024,
  };
  std::array<uint8_t, buffer_length> buffer_;
  std::unique_ptr<local_datagram_server> server_;
};
