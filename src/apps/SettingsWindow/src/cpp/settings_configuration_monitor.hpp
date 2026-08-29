#pragma once

#include "connected_devices.hpp"
#include "json_utility.hpp"
#include "monitor/configuration_monitor.hpp"
#include "settings.hpp"
#include "settings_configuration_snapshot.hpp"
#include "settings_remembered_device_properties.hpp"
#include <mutex>

class settings_configuration_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  settings_configuration_monitor(const settings_configuration_monitor&) = delete;

  settings_configuration_monitor(
      krbn_core_configuration_updated_t callback,
      krbn_core_configuration_load_state_changed_t load_state_changed_callback)
      : dispatcher_client(),
        callback_(callback),
        load_state_changed_callback_(load_state_changed_callback) {
  }

  ~settings_configuration_monitor() override {
    unregister_callbacks_and_detach();
  }

  void unregister_callbacks_and_detach() {
    std::call_once(unregister_callbacks_and_detach_once_, [this] {
      detach_from_dispatcher([this] {
        stop();
      });
    });
  }

  void start() {
    if (monitor_) {
      return;
    }

    monitor_ = std::make_unique<krbn::configuration_monitor>(
        krbn::constants::get_user_core_configuration_file_path().string(),
        geteuid(),
        krbn::core_configuration::error_handling::loose);

    monitor_->core_configuration_updated.connect([this](auto&& weak_core_configuration) {
      {
        std::lock_guard<std::mutex> lock(core_configuration_mutex_);
        weak_core_configuration_ = weak_core_configuration;
      }

      if (auto core_configuration = weak_core_configuration.lock()) {
        invoke_callback(*core_configuration);
      }
    });

    monitor_->load_state_changed.connect([this](auto load_state) {
      switch (load_state) {
        case krbn::core_configuration::core_configuration::load_state::loaded:
          load_state_changed_callback_(krbn_core_configuration_load_state_loaded);
          break;
        case krbn::core_configuration::core_configuration::load_state::permission_error:
          load_state_changed_callback_(krbn_core_configuration_load_state_permission_error);
          break;
        case krbn::core_configuration::core_configuration::load_state::json_error:
          load_state_changed_callback_(krbn_core_configuration_load_state_json_error);
          break;
        case krbn::core_configuration::core_configuration::load_state::other_error:
          load_state_changed_callback_(krbn_core_configuration_load_state_other_error);
          break;
      }
    });

    monitor_->async_start();
  }

  void stop() {
    monitor_ = nullptr;
  }

  [[nodiscard]] std::weak_ptr<krbn::core_configuration::core_configuration> get_weak_core_configuration() const {
    std::lock_guard<std::mutex> lock(core_configuration_mutex_);
    return weak_core_configuration_;
  }

  void remember_connected_devices(const krbn::connected_devices& connected_devices) {
    // The process-wide singleton retains device properties for disconnected devices
    // while this monitor is destroyed and recreated across sleep and wake.
    if (settings_remembered_device_properties::get_instance().remember_connected_devices(connected_devices)) {
      if (auto core_configuration = get_weak_core_configuration().lock()) {
        invoke_callback(*core_configuration);
      }
    }
  }

private:
  void invoke_callback(const krbn::core_configuration::core_configuration& core_configuration) const {
    auto json = krbn::json_utility::dump(
        settings_configuration_snapshot(core_configuration)
            .to_json());
    callback_(json.data(), json.size());
  }

  std::unique_ptr<krbn::configuration_monitor> monitor_;
  mutable std::mutex core_configuration_mutex_;
  std::weak_ptr<krbn::core_configuration::core_configuration> weak_core_configuration_;
  const krbn_core_configuration_updated_t callback_;
  const krbn_core_configuration_load_state_changed_t load_state_changed_callback_;
  std::once_flag unregister_callbacks_and_detach_once_;
};
