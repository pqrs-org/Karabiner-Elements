#pragma once

#include "constants.hpp"
#include "logger.hpp"
#include "notification_center.hpp"
#include "receiver.hpp"
#include "session.hpp"

class connection_manager final {
public:
  connection_manager(const connection_manager&) = delete;

  connection_manager(void) : exit_loop_(false),
                             session_state_(session::state::none),
                             exit_receiver_starter_(true),
                             sigusr1_source_(0) {
    thread_ = std::thread([this] { this->worker(); });

    notification_center::observe_distributed_notification(this,
                                                          static_console_user_socket_directory_is_ready_callback,
                                                          constants::get_distributed_notification_console_user_socket_directory_is_ready());
  }

  ~connection_manager(void) {
    exit_loop_ = true;
    exit_receiver_starter_ = true;

    receiver_starter_thread_.join();
    thread_.join();

    logger::get_logger().info("connection_manager is destructed");
  }

private:
  void worker(void) {
    while (!exit_loop_) {
      auto state = session::get_state();
      if (session_state_ != state) {
        session_state_ = state;

        exit_receiver_starter_ = true;
        if (receiver_starter_thread_.joinable()) {
          receiver_starter_thread_.join();
        }
        receiver_.stop();

        if (session_state_ == session::state::active) {
          exit_receiver_starter_ = false;
          receiver_starter_thread_ = std::thread([this] { this->receiver_starter_worker(); });
        }
      }

      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  void receiver_starter_worker(void) {
    while (!exit_receiver_starter_) {
      try {
        receiver_.start();
        break;
      } catch (...) {
        logger::get_logger().error("failed to start receiver_. retrying...");
      }

      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  static void static_console_user_socket_directory_is_ready_callback(CFNotificationCenterRef center,
                                                                     void* observer,
                                                                     CFStringRef notification_name,
                                                                     const void* observed_object,
                                                                     CFDictionaryRef user_info) {
    auto self = static_cast<connection_manager*>(observer);
    self->console_user_socket_directory_is_ready_callback();
  }

  void console_user_socket_directory_is_ready_callback(void) {
    logger::get_logger().info("connection_manager::console_user_socket_directory_is_ready_callback");
    // restart receiver.
    session_state_ = session::state::none;
  }

  std::thread thread_;
  volatile bool exit_loop_;
  volatile session::state session_state_;

  std::thread receiver_starter_thread_;
  volatile bool exit_receiver_starter_;
  receiver receiver_;

  dispatch_source_t sigusr1_source_;
};
