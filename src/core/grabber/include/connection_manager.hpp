#pragma once

#include "constants.hpp"
#include "device_grabber.hpp"
#include "event_manipulator.hpp"
#include "logger.hpp"
#include "notification_center.hpp"
#include "receiver.hpp"
#include "session.hpp"
#include <sys/stat.h>

class connection_manager final {
public:
  connection_manager(const connection_manager&) = delete;

  connection_manager(manipulator::event_manipulator& event_manipulator,
                     device_grabber& device_grabber) : event_manipulator_(event_manipulator),
                                                       device_grabber_(device_grabber),
                                                       queue_(dispatch_queue_create(nullptr, nullptr)),
                                                       timer_(0),
                                                       last_uid_(0) {
    timer_ = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue_);
    if (!timer_) {
      logger::get_logger().error("failed to dispatch_source_create");
    } else {
      dispatch_source_set_timer(timer_, dispatch_time(DISPATCH_TIME_NOW, 1.0 * NSEC_PER_SEC), 1.0 * NSEC_PER_SEC, 0);
      dispatch_source_set_event_handler(timer_, ^{
        if (auto uid = session::get_current_console_user_id()) {
          if (last_uid_ != *uid) {
            last_uid_ = *uid;
            logger::get_logger().info("current_console_user_id: {0}", *uid);

            event_manipulator_.relaunch_event_dispatcher();

            receiver_ = nullptr;
            receiver_ = std::make_unique<receiver>(event_manipulator_, device_grabber_);
          }
        }
      });
      dispatch_resume(timer_);
    }
  }

  ~connection_manager(void) {
    if (timer_) {
      dispatch_source_cancel(timer_);
      dispatch_release(timer_);
      timer_ = 0;
    }

    receiver_ = nullptr;

    dispatch_release(queue_);
  }

private:
  manipulator::event_manipulator& event_manipulator_;
  device_grabber& device_grabber_;

  dispatch_queue_t queue_;
  dispatch_source_t timer_;

  uid_t last_uid_;

  std::unique_ptr<receiver> receiver_;
};
