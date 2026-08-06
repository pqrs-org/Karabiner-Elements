#pragma once

#include "monitor/configuration_monitor.hpp"
#include "settings.hpp"
#include <atomic>
#include <pqrs/thread_wait.hpp>

class settings_configuration_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  settings_configuration_monitor(const settings_configuration_monitor&) = delete;

  settings_configuration_monitor()
      : dispatcher_client() {
    start(true);
  }

  ~settings_configuration_monitor() override {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  void start() {
    start(false);
  }

  void stop() {
    monitor_ = nullptr;
  }

  [[nodiscard]] std::weak_ptr<krbn::core_configuration::core_configuration> get_weak_core_configuration() const {
    return weak_core_configuration_;
  }

  void set_core_configuration_updated_callback(krbn_core_configuration_updated_t callback) {
    callback_.store(callback);
  }

private:
  void start(bool wait_for_initial_update) {
    if (monitor_) {
      return;
    }

    monitor_ = std::make_unique<krbn::configuration_monitor>(
        krbn::constants::get_user_core_configuration_file_path(),
        geteuid(),
        krbn::core_configuration::error_handling::loose);

    std::shared_ptr<pqrs::thread_wait> wait;
    if (wait_for_initial_update) {
      wait = pqrs::make_thread_wait();
    }

    monitor_->core_configuration_updated.connect([this, wait](auto&& weak_core_configuration) {
      weak_core_configuration_ = weak_core_configuration;

      if (auto callback = callback_.load()) {
        callback();
      }

      if (wait) {
        wait->notify();
      }
    });

    monitor_->async_start();

    if (wait) {
      wait->wait_notice();
    }
  }

  std::unique_ptr<krbn::configuration_monitor> monitor_;
  std::weak_ptr<krbn::core_configuration::core_configuration> weak_core_configuration_;
  std::atomic<krbn_core_configuration_updated_t> callback_{nullptr};
};
