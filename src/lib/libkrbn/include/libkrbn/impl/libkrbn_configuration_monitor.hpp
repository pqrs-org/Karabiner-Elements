#pragma once

#include "libkrbn/libkrbn.h"
#include "libkrbn_callback_manager.hpp"
#include "monitor/configuration_monitor.hpp"

class libkrbn_configuration_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  libkrbn_configuration_monitor(const libkrbn_configuration_monitor&) = delete;

  libkrbn_configuration_monitor(void)
      : dispatcher_client() {
    monitor_ = std::make_unique<krbn::configuration_monitor>(
        krbn::constants::get_user_core_configuration_file_path(),
        geteuid(),
        krbn::core_configuration::error_handling::loose);

    auto wait = pqrs::make_thread_wait();

    monitor_->core_configuration_updated.connect([this, wait](auto&& weak_core_configuration) {
      weak_core_configuration_ = weak_core_configuration;

      for (const auto& c : callback_manager_.get_callbacks()) {
        c();
      }

      wait->notify();
    });

    monitor_->async_start();

    wait->wait_notice();
  }

  ~libkrbn_configuration_monitor(void) {
    detach_from_dispatcher([this] {
      monitor_ = nullptr;
    });
  }

  std::weak_ptr<krbn::core_configuration::core_configuration> get_weak_core_configuration(void) const {
    return weak_core_configuration_;
  }

  void register_libkrbn_core_configuration_updated_callback(libkrbn_core_configuration_updated callback) {
    enqueue_to_dispatcher([this, callback] {
      callback_manager_.register_callback(callback);
    });
  }

  void unregister_libkrbn_core_configuration_updated_callback(libkrbn_core_configuration_updated callback) {
    enqueue_to_dispatcher([this, callback] {
      callback_manager_.unregister_callback(callback);
    });
  }

private:
  std::unique_ptr<krbn::configuration_monitor> monitor_;
  std::weak_ptr<krbn::core_configuration::core_configuration> weak_core_configuration_;
  libkrbn_callback_manager<libkrbn_core_configuration_updated> callback_manager_;
};
