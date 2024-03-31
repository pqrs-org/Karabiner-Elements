#pragma once

#include "constants.hpp"
#include "libkrbn/libkrbn.h"
#include "libkrbn_callback_manager.hpp"
#include "monitor/connected_devices_monitor.hpp"

class libkrbn_connected_devices_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  libkrbn_connected_devices_monitor(const libkrbn_connected_devices_monitor&) = delete;

  libkrbn_connected_devices_monitor(void)
      : dispatcher_client() {
    monitor_ = std::make_unique<krbn::connected_devices_monitor>(
        krbn::constants::get_devices_json_file_path());

    auto wait = pqrs::make_thread_wait();

    monitor_->connected_devices_updated.connect([this, wait](auto&& weak_connected_devices) {
      weak_connected_devices_ = weak_connected_devices;

      for (const auto& c : callback_manager_.get_callbacks()) {
        c();
      }

      wait->notify();
    });

    monitor_->async_start();

    wait->wait_notice();
  }

  ~libkrbn_connected_devices_monitor(void) {
    detach_from_dispatcher([this] {
      monitor_ = nullptr;
    });
  }

  std::weak_ptr<const krbn::connected_devices::connected_devices> get_weak_connected_devices(void) const {
    return weak_connected_devices_;
  }

  void register_libkrbn_connected_devices_updated_callback(libkrbn_connected_devices_updated callback) {
    enqueue_to_dispatcher([this, callback] {
      callback_manager_.register_callback(callback);
    });
  }

  void unregister_libkrbn_connected_devices_updated_callback(libkrbn_connected_devices_updated callback) {
    enqueue_to_dispatcher([this, callback] {
      callback_manager_.unregister_callback(callback);
    });
  }

private:
  std::unique_ptr<krbn::connected_devices_monitor> monitor_;
  std::weak_ptr<const krbn::connected_devices::connected_devices> weak_connected_devices_;
  libkrbn_callback_manager<libkrbn_connected_devices_updated> callback_manager_;
};
