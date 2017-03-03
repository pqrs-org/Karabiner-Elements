#pragma once

#include "gcd_utility.hpp"
#include <spdlog/spdlog.h>

namespace krbn {
class process_monitor final {
public:
  typedef std::function<void(void)> exit_callback;

  process_monitor(spdlog::logger& logger,
                  pid_t pid,
                  const exit_callback& exit_callback) : logger_(logger),
                                                        exit_callback_(exit_callback),
                                                        monitor_(0) {
    monitor_ = dispatch_source_create(DISPATCH_SOURCE_TYPE_PROC, pid, DISPATCH_PROC_EXIT, dispatch_get_main_queue());
    if (!monitor_) {
      logger_.error("failed to dispatch_source_create @ {0}", __PRETTY_FUNCTION__);
    } else {
      dispatch_source_set_event_handler(monitor_, ^{
        logger::get_logger().info("pid:{0} is exited", pid);

        if (exit_callback_) {
          exit_callback_();
        }

        release();
      });
      dispatch_resume(monitor_);
    }
  }

  ~process_monitor(void) {
    release();
  }

private:
  void release(void) {
    // Release monitor_ in main thread to avoid callback invocations after object has been destroyed.
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (monitor_) {
        dispatch_source_cancel(monitor_);
        dispatch_release(monitor_);
        monitor_ = 0;
      }
    });
  }

  spdlog::logger& logger_;
  exit_callback exit_callback_;

  dispatch_source_t monitor_;
};
}
