#pragma once

#include "libkrbn/libkrbn.h"
#include "monitor/system_preferences_monitor.hpp"

namespace {
class libkrbn_system_preferences_monitor final {
public:
  libkrbn_system_preferences_monitor(const libkrbn_system_preferences_monitor&) = delete;

  libkrbn_system_preferences_monitor(libkrbn_system_preferences_monitor_callback callback,
                                     void* refcon) {
    krbn::logger::get_logger()->info(__func__);

    // configuration_monitor_

    configuration_monitor_ = std::make_shared<krbn::configuration_monitor>(
        krbn::constants::get_user_core_configuration_file_path());

    configuration_monitor_->async_start();

    // monitor_

    monitor_ = std::make_unique<krbn::system_preferences_monitor>(configuration_monitor_);

    monitor_->system_preferences_changed.connect([callback, refcon](auto&& system_preferences) {
      if (callback) {
        libkrbn_system_preferences v;
        v.keyboard_fn_state = system_preferences.get_keyboard_fn_state();
        callback(&v, refcon);
      }
    });

    monitor_->async_start();
  }

  ~libkrbn_system_preferences_monitor(void) {
    krbn::logger::get_logger()->info(__func__);

    monitor_ = nullptr;
    configuration_monitor_ = nullptr;
  }

private:
  std::shared_ptr<krbn::configuration_monitor> configuration_monitor_;
  std::unique_ptr<krbn::system_preferences_monitor> monitor_;
};
} // namespace
