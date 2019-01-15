#pragma once

#include "libkrbn/libkrbn.h"
#include <pqrs/osx/frontmost_application_monitor.hpp>

class libkrbn_frontmost_application_monitor final {
public:
  libkrbn_frontmost_application_monitor(const libkrbn_frontmost_application_monitor&) = delete;

  libkrbn_frontmost_application_monitor(libkrbn_frontmost_application_monitor_callback callback,
                                        void* refcon) {
    monitor_ = std::make_unique<pqrs::osx::frontmost_application_monitor::monitor>(
        pqrs::dispatcher::extra::get_shared_dispatcher());

    monitor_->frontmost_application_changed.connect([callback, refcon](auto&& application_ptr) {
      if (application_ptr && callback) {
        std::string bundle_identifier = application_ptr->get_bundle_identifier().value_or("");
        std::string file_path = application_ptr->get_file_path().value_or("");

        callback(bundle_identifier.c_str(),
                 file_path.c_str(),
                 refcon);
      }
    });

    monitor_->async_start();
  }

private:
  std::unique_ptr<pqrs::osx::frontmost_application_monitor::monitor> monitor_;
};
