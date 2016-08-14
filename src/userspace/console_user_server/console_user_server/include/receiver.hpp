#pragma once

#include "constants.hpp"
#include "io_hid_post_event_wrapper.hpp"
#include "local_datagram_server.hpp"
#include "logger.hpp"
#include "system_preferences.hpp"
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

      if (!ec) {
        switch (buffer_[0]) {
        case KRBN_OPERATION_TYPE_POST_MODIFIER_FLAGS:
          if (n != sizeof(krbn_operation_type_post_modifier_flags)) {
            logger::get_logger().error("invalid size for KRBN_OPERATION_TYPE_POST_MODIFIER_FLAGS");
          } else {
            auto p = reinterpret_cast<krbn_operation_type_post_modifier_flags*>(&(buffer_[0]));
            io_hid_post_event_wrapper_.post_modifier_flags(p->flags);
          }
          break;

        case KRBN_OPERATION_TYPE_POST_KEY:
          if (n != sizeof(krbn_operation_type_post_key)) {
            logger::get_logger().error("invalid size for KRBN_OPERATION_TYPE_POST_KEY");
          } else {
            auto p = reinterpret_cast<krbn_operation_type_post_key*>(&(buffer_[0]));
            io_hid_post_event_wrapper_.post_modifier_flags(p->flags);

            bool fn_pressed = (p->flags & NX_SECONDARYFNMASK);
            bool standard_function_key = false;
            if (system_preferences::get_keyboard_fn_state()) {
              // "Use all F1, F2, etc. keys as standard function keys."
              standard_function_key = !fn_pressed;
            } else {
              standard_function_key = fn_pressed;
            }

            switch (p->key_code) {
            case KRBN_KEY_CODE_F1:
              if (standard_function_key) {
                io_hid_post_event_wrapper_.post_key(p->key_code, p->event_type, p->flags, false);
              } else {
                io_hid_post_event_wrapper_.post_aux_key(NX_KEYTYPE_BRIGHTNESS_DOWN, p->event_type, p->flags, false);
              }
              break;

            case KRBN_KEY_CODE_F2:
              if (standard_function_key) {
                io_hid_post_event_wrapper_.post_key(p->key_code, p->event_type, p->flags, false);
              } else {
                io_hid_post_event_wrapper_.post_aux_key(NX_KEYTYPE_BRIGHTNESS_UP, p->event_type, p->flags, false);
              }
              break;

            default:
              io_hid_post_event_wrapper_.post_key(p->key_code, p->event_type, p->flags, false);
            }
          }
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
