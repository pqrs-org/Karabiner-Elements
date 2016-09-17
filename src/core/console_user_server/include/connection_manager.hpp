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
                             is_session_active_(false),
                             exit_receiver_starter_(true) {
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
      if (auto is_active = session::is_active()) {
        if (is_session_active_ != *is_active) {
          is_session_active_ = *is_active;

          exit_receiver_starter_ = true;
          if (receiver_starter_thread_.joinable()) {
            receiver_starter_thread_.join();
          }
          receiver_.stop();

          if (is_session_active_) {
            exit_receiver_starter_ = false;
            receiver_starter_thread_ = std::thread([this] { this->receiver_starter_worker(); });
          }
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
    is_session_active_ = false;
  }

  std::thread thread_;
  std::atomic<bool> exit_loop_;
  std::atomic<bool> is_session_active_;

  std::thread receiver_starter_thread_;
  std::atomic<bool> exit_receiver_starter_;
  receiver receiver_;
};
