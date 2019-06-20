#pragma once

#include "libkrbn/libkrbn.h"
#include "monitor/kextd_state_monitor.hpp"

class libkrbn_kextd_state_monitor final {
public:
  libkrbn_kextd_state_monitor(const libkrbn_kextd_state_monitor&) = delete;

  libkrbn_kextd_state_monitor(libkrbn_kextd_state_monitor_kext_load_result_changed_callback callback,
                              void* refcon) {
    krbn::logger::get_logger()->info(__func__);

    monitor_ = std::make_unique<krbn::kextd_state_monitor>(
        krbn::constants::get_kextd_state_json_file_path());

    monitor_->kext_load_result_changed.connect([callback, refcon](auto&& result) {
      if (callback) {
        callback(result, refcon);
      }
    });

    monitor_->async_start();
  }

  ~libkrbn_kextd_state_monitor(void) {
    krbn::logger::get_logger()->info(__func__);
  }

private:
  std::unique_ptr<krbn::kextd_state_monitor> monitor_;
};
