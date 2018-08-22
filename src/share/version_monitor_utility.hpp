#pragma once

#include "constants.hpp"
#include "version_monitor.hpp"

namespace krbn {
class version_monitor_utility final {
public:
  static std::shared_ptr<version_monitor> make_version_monitor_stops_main_run_loop_when_version_changed(void) {
    auto monitor = std::make_shared<version_monitor>(constants::get_version_file_path());

    monitor->changed.connect([](auto&& version) {
      dispatch_async(dispatch_get_main_queue(), ^{
        CFRunLoopStop(CFRunLoopGetCurrent());
      });
    });

    monitor->async_start();

    return monitor;
  }
};
} // namespace krbn
