#pragma once

#include "logger.hpp"
#include "session.hpp"

class userspace_connection_manager final {
public:
  userspace_connection_manager(void) {
    auto logger = logger::get_logger();

    timer_ = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
    if (!timer_) {
      logger->error("failed to dispatch_source_create");
    }

    dispatch_source_set_timer(timer_, dispatch_time(DISPATCH_TIME_NOW, 1.0 * NSEC_PER_SEC), 1.0 * NSEC_PER_SEC, 0);
    dispatch_source_set_event_handler(timer_, ^{
      uid_t uid = 0;
      session::get_current_console_user_id(uid);
      logger->info("uid {0}", uid);
    });

    dispatch_resume(timer_);
  }

private:
  dispatch_source_t timer_;
};
