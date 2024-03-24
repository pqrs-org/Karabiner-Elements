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

class libkrbn_configuration_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  libkrbn_configuration_monitor(const libkrbn_configuration_monitor&) = delete;

  libkrbn_configuration_monitor(void)
      : dispatcher_client() {
    monitor_ = std::make_unique<krbn::configuration_monitor>(
        krbn::constants::get_user_core_configuration_file_path(),
        geteuid());

    auto wait = pqrs::make_thread_wait();

    monitor_->core_configuration_updated.connect([this, wait](auto&& weak_core_configuration) {
      for (const auto& c : libkrbn_core_configuration_updated_callbacks_) {
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

  void register_libkrbn_core_configuration_updated_callback(libkrbn_core_configuration_updated callback) {
    enqueue_to_dispatcher([this, callback] {
      libkrbn_core_configuration_updated_callbacks_.push_back(callback);
    });
  }

  void unregister_libkrbn_core_configuration_updated_callback(libkrbn_core_configuration_updated callback) {
    enqueue_to_dispatcher([this, callback] {
      libkrbn_core_configuration_updated_callbacks_.erase(std::remove_if(std::begin(libkrbn_core_configuration_updated_callbacks_),
                                                                         std::end(libkrbn_core_configuration_updated_callbacks_),
                                                                         [&](auto& c) {
                                                                           return c == callback;
                                                                         }),
                                                          std::end(libkrbn_core_configuration_updated_callbacks_));
    });
  }

private:
  std::unique_ptr<krbn::configuration_monitor> monitor_;
  std::vector<libkrbn_core_configuration_updated> libkrbn_core_configuration_updated_callbacks_;
};
