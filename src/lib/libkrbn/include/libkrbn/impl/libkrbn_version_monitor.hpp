#pragma once

#include "monitor/version_monitor.hpp"

class libkrbn_version_monitor final {
public:
  libkrbn_version_monitor(const libkrbn_version_monitor&) = delete;

  libkrbn_version_monitor(libkrbn_version_monitor_callback callback,
                          void* refcon) {
    monitor_ = std::make_unique<krbn::version_monitor>(krbn::constants::get_version_file_path());

    monitor_->changed.connect([callback, refcon](auto&& version) {
      if (callback) {
        callback(refcon);
      }
    });

    monitor_->async_start();
  }

private:
  std::unique_ptr<krbn::version_monitor> monitor_;
};
