#pragma once

#include "constants.hpp"
#include "libkrbn/libkrbn.h"
#include "monitor/connected_devices_monitor.hpp"

namespace {
class libkrbn_connected_devices_class final {
public:
  libkrbn_connected_devices_class(const krbn::connected_devices::connected_devices& connected_devices) : connected_devices_(connected_devices) {
  }

  krbn::connected_devices::connected_devices& get_connected_devices(void) {
    return connected_devices_;
  }

private:
  krbn::connected_devices::connected_devices connected_devices_;
};

class libkrbn_connected_devices_monitor final {
public:
  libkrbn_connected_devices_monitor(const libkrbn_connected_devices_monitor&) = delete;

  libkrbn_connected_devices_monitor(libkrbn_connected_devices_monitor_callback callback, void* refcon) {
    krbn::logger::get_logger()->info(__func__);

    monitor_ = std::make_unique<krbn::connected_devices_monitor>(
        krbn::constants::get_devices_json_file_path());

    monitor_->connected_devices_updated.connect([callback, refcon](auto&& weak_connected_devices) {
      if (auto connected_devices = weak_connected_devices.lock()) {
        if (callback) {
          auto* p = new libkrbn_connected_devices_class(*connected_devices);
          callback(p, refcon);
        }
      }
    });

    monitor_->async_start();
  }

  ~libkrbn_connected_devices_monitor(void) {
    krbn::logger::get_logger()->info(__func__);
  }

private:
  std::unique_ptr<krbn::connected_devices_monitor> monitor_;
};
} // namespace
