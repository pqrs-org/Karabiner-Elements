#pragma once

#include "constants.hpp"
#include "logger.hpp"
#include "receiver.hpp"
#include "session.hpp"

class connection_manager final {
public:
  connection_manager(void) : exit_loop_(false),
                             session_state_(session::state::none),
                             exit_receiver_starter_(true) {
    thread_ = std::thread([this] { this->worker(); });
  }

  ~connection_manager(void) {
    exit_loop_ = true;
    exit_receiver_starter_ = true;

    receiver_starter_thread_.join();
    thread_.join();
  }

private:
  void worker(void) {
    while (!exit_loop_) {
      auto state = session::get_state();
      if (session_state_ != state) {
        session_state_ = state;

        if (session_state_ == session::state::active) {
          exit_receiver_starter_ = false;
          receiver_starter_thread_ = std::thread([this] { this->receiver_starter_worker(); });

        } else {
          exit_receiver_starter_ = true;
          receiver_starter_thread_.join();
          receiver_.stop();
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

  std::thread thread_;
  volatile bool exit_loop_;

  session::state session_state_;

  std::thread receiver_starter_thread_;
  volatile bool exit_receiver_starter_;
  receiver receiver_;
};
