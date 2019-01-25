#pragma once

#include "libkrbn/libkrbn.h"
#include <pqrs/osx/file_monitor.hpp>

class libkrbn_file_monitor final {
public:
  libkrbn_file_monitor(const libkrbn_file_monitor&) = delete;

  libkrbn_file_monitor(const std::string& file_path,
                       libkrbn_file_monitor_callback callback,
                       void* refcon) {
    krbn::logger::get_logger()->info(__func__);

    std::vector<std::string> targets = {
        file_path,
    };
    monitor_ = std::make_unique<pqrs::osx::file_monitor>(pqrs::dispatcher::extra::get_shared_dispatcher(),
                                                         targets);

    monitor_->file_changed.connect([callback, refcon](auto&& changed_file_path,
                                                      auto&& changed_file_body) {
      if (callback) {
        callback(changed_file_path.c_str(), refcon);
      }
    });

    monitor_->async_start();
  }

  ~libkrbn_file_monitor(void) {
    krbn::logger::get_logger()->info(__func__);
  }

private:
  std::unique_ptr<pqrs::osx::file_monitor> monitor_;
};
