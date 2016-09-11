#pragma once

#include "configuration_manager.hpp"
#include "constants.hpp"
#include "grabber_client.hpp"
#include "keyboard_event_output_manager.hpp"
#include "local_datagram_server.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <vector>

class receiver final {
public:
  receiver(const receiver&) = delete;

  receiver(void) : exit_loop_(false) {
    const size_t buffer_length = 8 * 1024;
    buffer_.resize(buffer_length);
  }

  void start(void) {
    if (server_) {
      logger::get_logger().error("receiver is already running");
      return;
    }

    keyboard_event_output_manager_.stop_key_repeat();

    const char* path = constants::get_console_user_socket_file_path();
    unlink(path);
    server_ = std::make_unique<local_datagram_server>(path);
    chmod(path, 0600);

    grabber_client_ = std::make_unique<grabber_client>();
    grabber_client_->connect();

    configuration_manager_ = std::make_unique<configuration_manager>(logger::get_logger(),
                                                                     constants::get_configuration_directory(),
                                                                     *grabber_client_,
                                                                     keyboard_event_output_manager_);

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
    configuration_manager_.reset(nullptr);
    grabber_client_.reset(nullptr);
    server_.reset(nullptr);

    keyboard_event_output_manager_.stop_key_repeat();

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
        switch (krbn::operation_type(buffer_[0])) {
        case krbn::operation_type::stop_key_repeat:
          keyboard_event_output_manager_.stop_key_repeat();
          break;

        case krbn::operation_type::post_modifier_flags:
          if (n != sizeof(krbn::operation_type_post_modifier_flags_struct)) {
            logger::get_logger().error("invalid size for krbn::operation_type::post_modifier_flags");
          } else {
            auto p = reinterpret_cast<krbn::operation_type_post_modifier_flags_struct*>(&(buffer_[0]));
            keyboard_event_output_manager_.post_modifier_flags(p->key_code, p->flags);
          }
          break;

        case krbn::operation_type::post_key:
          if (n != sizeof(krbn::operation_type_post_key_struct)) {
            logger::get_logger().error("invalid size for krbn::operation_type::post_key");
          } else {
            auto p = reinterpret_cast<krbn::operation_type_post_key_struct*>(&(buffer_[0]));
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
  std::unique_ptr<configuration_manager> configuration_manager_;
  std::thread thread_;
  volatile bool exit_loop_;

  keyboard_event_output_manager keyboard_event_output_manager_;
};
