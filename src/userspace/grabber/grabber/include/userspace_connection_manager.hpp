#pragma once

#include "constants.hpp"
#include "logger.hpp"
#include "session.hpp"

class userspace_connection_manager final {
public:
  userspace_connection_manager(void) : timer_(0), last_uid_(0) {
    auto logger = logger::get_logger();

    prepare_socket_directory(0);

    timer_ = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
    if (!timer_) {
      logger->error("failed to dispatch_source_create");
    }

    dispatch_source_set_timer(timer_, dispatch_time(DISPATCH_TIME_NOW, 1.0 * NSEC_PER_SEC), 1.0 * NSEC_PER_SEC, 0);
    dispatch_source_set_event_handler(timer_, ^{
      uid_t uid = 0;
      session::get_current_console_user_id(uid);

      if (last_uid_ != uid) {
        last_uid_ = uid;
        logger->info("current_console_user_id: {0}", uid);
        prepare_socket_directory(uid);
      }
    });

    dispatch_resume(timer_);
  }

  void prepare_socket_directory(uid_t uid) {
    unlink(constants::get_console_user_socket_file_path());
    chmod(constants::get_console_user_socket_directory(), 0700);
    chown(constants::get_console_user_socket_directory(), uid, 0);
  }

private:
  dispatch_source_t timer_;
  uid_t last_uid_;
};
