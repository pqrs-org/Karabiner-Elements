#pragma once

#include "constants.hpp"
#include "io_hid_post_event_wrapper.hpp"
#include "local_datagram_server.hpp"
#include "logger.hpp"
#include "userspace_defs.h"

class receiver final {
public:
  receiver(void) : exit_loop_(false) {}

  void start(void) {
    std::lock_guard<std::mutex> guard(mutex_);

    if (server_) {
      logger::get_logger().error("receiver is already running");
      return;
    }

    const char* path = constants::get_console_user_socket_file_path();
    unlink(path);
    server_ = std::make_unique<local_datagram_server>(path);
    chmod(path, 0600);

    exit_loop_ = false;
    thread_ = std::thread([this] { this->worker(); });

    logger::get_logger().info("receiver is started");
  }

  void stop(void) {
    std::lock_guard<std::mutex> guard(mutex_);

    if (!thread_.joinable()) {
      return;
    }
    if (!server_) {
      return;
    }

    exit_loop_ = true;
    thread_.join();
    server_.reset(nullptr);

    logger::get_logger().info("receiver is stopped");
  }

  void worker(void) {
    if (!server_) {
      return;
    }

    while (!exit_loop_) {
      boost::system::error_code ec;
      std::size_t n = server_->receive(boost::asio::buffer(buffer_), boost::posix_time::seconds(1), ec);

      if (!ec && n > 0) {
        --n;

        switch (buffer_[0]) {
        case KRBN_OP_TYPE_POST_MODIFIER_FLAGS:
          if (n == sizeof(IOOptionBits)) {
            __block IOOptionBits flags;
            memcpy(&flags, &(buffer_[1]), sizeof(flags));
            io_hid_post_event_wrapper_.post_modifier_flags(flags);
          }
          break;

        case KRBN_OP_TYPE_POST_KEY:
          break;

        default:
          break;
        }
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
  std::mutex mutex_;
  volatile bool exit_loop_;

  io_hid_post_event_wrapper io_hid_post_event_wrapper_;
};
