#pragma once

#include "system_preferences.hpp"
#include <spdlog/spdlog.h>

class system_preferences_monitor final {
public:
  typedef std::function<void(const system_preferences::values& values)> values_updated_callback;

  system_preferences_monitor(spdlog::logger& logger,
                             const values_updated_callback& callback) : logger_(logger),
                                                                        callback_(callback),
                                                                        queue_(dispatch_queue_create(nullptr, nullptr)),
                                                                        timer_(nullptr) {
    if (callback_) {
      callback_(values_);
    }

    timer_ = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue_);
    if (!timer_) {
      logger_.error("failed to dispatch_source_create @ {0}", __PRETTY_FUNCTION__);
    } else {
      dispatch_source_set_timer(timer_, dispatch_time(DISPATCH_TIME_NOW, 1.0 * NSEC_PER_SEC), 1.0 * NSEC_PER_SEC, 0);
      dispatch_source_set_event_handler(timer_, ^{
        system_preferences::values v;
        if (values_ != v) {
          logger_.info("system_preferences::values is updated.");

          values_ = v;
          if (callback_) {
            callback_(values_);
          }
        }
      });
      dispatch_resume(timer_);
    }
  }

  ~system_preferences_monitor(void) {
    if (timer_) {
      dispatch_source_cancel(timer_);
      dispatch_release(timer_);
      timer_ = nullptr;
    }

    dispatch_release(queue_);
    queue_ = nullptr;
  }

private:
  spdlog::logger& logger_;
  values_updated_callback callback_;

  dispatch_queue_t queue_;
  dispatch_source_t timer_;

  system_preferences::values values_;
};
