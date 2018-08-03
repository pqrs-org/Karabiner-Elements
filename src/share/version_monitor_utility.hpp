#pragma once

#include "version_monitor.hpp"

namespace krbn {
class version_monitor_utility final {
public:
  static void start_monitor_to_stop_run_loop_when_version_changed(void) {
    auto monitor = version_monitor::get_shared_instance();

    monitor->changed.connect([] {
      dispatch_async(dispatch_get_main_queue(), ^{
        CFRunLoopStop(CFRunLoopGetCurrent());
      });
    });

    monitor->start();
  }
};
} // namespace krbn
