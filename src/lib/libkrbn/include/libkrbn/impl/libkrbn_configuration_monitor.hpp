#pragma once

#include "libkrbn/libkrbn.h"
#include "monitor/configuration_monitor.hpp"

class libkrbn_core_configuration_class final {
public:
  libkrbn_core_configuration_class(std::shared_ptr<krbn::core_configuration::core_configuration> core_configuration) : core_configuration_(core_configuration) {
  }

  krbn::core_configuration::core_configuration& get_core_configuration(void) {
    return *core_configuration_;
  }

private:
  std::shared_ptr<krbn::core_configuration::core_configuration> core_configuration_;
};

class libkrbn_configuration_monitor final {
public:
  libkrbn_configuration_monitor(const libkrbn_configuration_monitor&) = delete;

  libkrbn_configuration_monitor(libkrbn_configuration_monitor_callback callback, void* refcon) {
    monitor_ = std::make_unique<krbn::configuration_monitor>(
        krbn::constants::get_user_core_configuration_file_path(),
        geteuid());

    auto wait = pqrs::make_thread_wait();

    monitor_->core_configuration_updated.connect([callback, refcon, wait](auto&& weak_core_configuration) {
      if (auto core_configuration = weak_core_configuration.lock()) {
        if (callback) {
          auto* p = new libkrbn_core_configuration_class(core_configuration);
          callback(p, refcon);
        }
      }
      wait->notify();
    });

    monitor_->async_start();

    wait->wait_notice();
  }

  ~libkrbn_configuration_monitor(void) {
    krbn::logger::get_logger()->info(__func__);
  }

private:
  std::unique_ptr<krbn::configuration_monitor> monitor_;
};
