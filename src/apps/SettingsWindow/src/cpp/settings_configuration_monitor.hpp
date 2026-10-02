#pragma once

#include "connected_devices.hpp"
#include "json_utility.hpp"
#include "monitor/configuration_monitor.hpp"
#include "settings.hpp"
#include "settings_configuration_store.hpp"
#include <mutex>
#include <pqrs/dispatcher.hpp>

class settings_configuration_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
  pqrs::dispatcher::extra::dispatcher_client_constructor_exception_guard dispatcher_client_constructor_guard_{*this};

public:
  settings_configuration_monitor(const settings_configuration_monitor&) = delete;

  settings_configuration_monitor(
      krbn_core_configuration_updated_t callback,
      krbn_core_configuration_load_state_changed_t load_state_changed_callback)
      : configuration_store_(std::make_shared<settings_configuration_store>(
            settings_configuration_store::callbacks{
                .updated = [callback](const auto& value) {
                  auto json = krbn::json_utility::dump(value);
                  callback(json.data(), json.size()); },
                .save = [](auto& configuration) { configuration.sync_save_to_file(); },
            })),
        load_state_changed_callback_(load_state_changed_callback) {
    dispatcher_client_constructor_guard_.initialize();
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
      if (auto core_configuration = weak_core_configuration.lock()) {
        configuration_store_->update(std::move(core_configuration));
      }
    });

    monitor_->load_state_changed.connect([this](auto load_state) {
      if (load_state != krbn::core_configuration::core_configuration::load_state::loaded) {
        configuration_store_->suspend_configuration();
      }
      // A successful load becomes writable only when update supplies its configuration.
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
    configuration_store_->stop();
    monitor_ = nullptr;
  }

  [[nodiscard]] pqrs::not_null_shared_ptr_t<settings_configuration_store> get_configuration_store() const {
    return configuration_store_;
  }

  void remember_connected_devices(const krbn::connected_devices& connected_devices) {
    configuration_store_->set_connected_devices(connected_devices);
  }

private:
  std::unique_ptr<krbn::configuration_monitor> monitor_;
  pqrs::not_null_shared_ptr_t<settings_configuration_store> configuration_store_;
  const krbn_core_configuration_load_state_changed_t load_state_changed_callback_;
  std::once_flag unregister_callbacks_and_detach_once_;
};
