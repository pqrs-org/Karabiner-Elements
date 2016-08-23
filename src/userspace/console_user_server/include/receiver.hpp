#pragma once

#include "constants.hpp"
#include "grabber_client.hpp"
#include "keyboard_event_output_manager.hpp"
#include "local_datagram_server.hpp"
#include "logger.hpp"
#include "userspace_defs.h"
#include <vector>

class receiver final {
public:
  receiver(void) : exit_loop_(false) {
    enum {
      buffer_length = 8 * 1024,
    };
    buffer_.resize(buffer_length);
  }

  void start(void) {
    if (server_) {
      logger::get_logger().error("receiver is already running");
      return;
    }

    const char* path = constants::get_console_user_socket_file_path();
    unlink(path);
    server_ = std::make_unique<local_datagram_server>(path);
    chmod(path, 0600);

    grabber_client_ = std::make_unique<grabber_client>();
    grabber_client_->connect();

    const uint32_t buffer[] = {
        kHIDUsage_KeyboardCapsLock, kHIDUsage_KeyboardDeleteOrBackspace,
        kHIDUsage_KeyboardEscape, kHIDUsage_KeyboardSpacebar,
    };
    grabber_client_->define_simple_modifications(buffer, sizeof(buffer) / sizeof(buffer[0]));

    exit_loop_ = false;
    thread_ = std::thread([this] { this->worker(); });

    logger::get_logger().info("receiver is started");
  }

  void stop(void) {
    unlink(constants::get_console_user_socket_file_path());

    if (!thread_.joinable()) {
      return;
    }
    if (!server_) {
      return;
    }

    exit_loop_ = true;
    thread_.join();
    server_.reset(nullptr);
    grabber_client_.reset(nullptr);

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
        switch (buffer_[0]) {
        case KRBN_OPERATION_TYPE_STOP_KEY_REPEAT:
          keyboard_event_output_manager_.stop_key_repeat();
          break;

        case KRBN_OPERATION_TYPE_POST_MODIFIER_FLAGS:
          if (n != sizeof(krbn_operation_type_post_modifier_flags)) {
            logger::get_logger().error("invalid size for KRBN_OPERATION_TYPE_POST_MODIFIER_FLAGS");
          } else {
            auto p = reinterpret_cast<krbn_operation_type_post_modifier_flags*>(&(buffer_[0]));
            keyboard_event_output_manager_.post_modifier_flags(p->flags);
          }
          break;

        case KRBN_OPERATION_TYPE_POST_KEY:
          if (n != sizeof(krbn_operation_type_post_key)) {
            logger::get_logger().error("invalid size for KRBN_OPERATION_TYPE_POST_KEY");
          } else {
            auto p = reinterpret_cast<krbn_operation_type_post_key*>(&(buffer_[0]));
            keyboard_event_output_manager_.post_key(p->key_code, p->event_type, p->flags);
          }
          break;

        default:
          break;
        }
      }
    }

    keyboard_event_output_manager_.stop_key_repeat();
  }

private:
  std::vector<uint8_t> buffer_;
  std::unique_ptr<local_datagram_server> server_;
  std::unique_ptr<grabber_client> grabber_client_;
  std::thread thread_;
  volatile bool exit_loop_;

  keyboard_event_output_manager keyboard_event_output_manager_;
};
