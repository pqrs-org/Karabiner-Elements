#pragma once

#include "constants.hpp"
#include "session.hpp"

class connection_manager final {
public:
  connection_manager(void) : timer_(0), session_state_(session::state::none) {
    timer_ = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
    if (timer_) {
      dispatch_source_set_timer(timer_, dispatch_time(DISPATCH_TIME_NOW, 1.0 * NSEC_PER_SEC), 1.0 * NSEC_PER_SEC, 0);
      dispatch_source_set_event_handler(timer_, ^{
        auto state = session::get_state();
        if (session_state_ != state) {
          session_state_ = state;
          std::cout << "session_state is changed" << std::endl;
        }
      });

      dispatch_resume(timer_);
    }
  }

private:
  dispatch_source_t timer_;
  session::state session_state_;
};
