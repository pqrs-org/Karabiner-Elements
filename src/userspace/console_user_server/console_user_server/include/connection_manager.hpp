#pragma once

#include "constants.hpp"
#include "logger.hpp"
#include "receiver.hpp"
#include "session.hpp"

class connection_manager final {
public:
  connection_manager(void) : timer_(0),
                             session_state_(session::state::none),
                             exit_receiver_starter_(true) {
    timer_ = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
    if (timer_) {
      dispatch_source_set_timer(timer_, dispatch_time(DISPATCH_TIME_NOW, 1.0 * NSEC_PER_SEC), 1.0 * NSEC_PER_SEC, 0);
      dispatch_source_set_event_handler(timer_, ^{
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
      });

      dispatch_resume(timer_);
    }
  }

  ~connection_manager(void) {
    exit_receiver_starter_ = true;
    receiver_starter_thread_.join();

    if (timer_) {
      dispatch_release(timer_);
      timer_ = 0;
    }
  }

private:
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

  dispatch_source_t timer_;
  session::state session_state_;

  std::thread receiver_starter_thread_;
  volatile bool exit_receiver_starter_;
  receiver receiver_;
};
