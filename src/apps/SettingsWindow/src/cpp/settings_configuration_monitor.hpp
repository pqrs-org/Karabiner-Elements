#pragma once

#include "monitor/configuration_monitor.hpp"
#include "settings.hpp"
#include "settings_callback_manager.hpp"

class settings_configuration_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  settings_configuration_monitor(const settings_configuration_monitor&) = delete;

  settings_configuration_monitor()
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

  ~settings_configuration_monitor() override {
    detach_from_dispatcher([this] {
      monitor_ = nullptr;
    });
  }

  [[nodiscard]] std::weak_ptr<krbn::core_configuration::core_configuration> get_weak_core_configuration() const {
    return weak_core_configuration_;
  }

  void register_krbn_core_configuration_updated_callback(krbn_core_configuration_updated_t callback) {
    enqueue_to_dispatcher([this, callback] {
      callback_manager_.register_callback(callback);
    });
  }

private:
  std::unique_ptr<krbn::configuration_monitor> monitor_;
  std::weak_ptr<krbn::core_configuration::core_configuration> weak_core_configuration_;
  settings_callback_manager<krbn_core_configuration_updated_t> callback_manager_;
};
