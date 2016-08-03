#pragma once

#include "constants.hpp"
#include "local_datagram_server.hpp"

class receiver final {
public:
  void start(void) {
    try {
      const char* path = constants::get_console_user_socket_file_path();
      unlink(path);
      server_ = std::make_unique<local_datagram_server>(path);
      chmod(path, 0600);

      thread_ = std::thread([this] { this->worker(); });
    } catch (...) {
      std::cout << "default exception";
    }
  }

  void stop(void) {
    if (!thread_.joinable()) {
      return;
    }
    if (!server_) {
      return;
    }

    server_->get_socket().cancel();
    thread_.join();
    server_.reset(nullptr);
  }

  void worker(void) {
    for (;;) {
      if (!server_) {
        break;
      }

      boost::system::error_code ec;
      std::size_t n = server_->receive(boost::asio::buffer(buffer_), boost::posix_time::seconds(1), ec);

      if (ec) {
        std::cout << "Receive error: " << ec.message() << "\n";
      } else {
        std::cout << n << std::endl;
      }
    }
  }

private:
  enum {
    buffer_length = 8 * 1024,
  };
  std::array<uint8_t, buffer_length> buffer_;
  std::unique_ptr<local_datagram_server> server_;
  std::thread thread_;
};
