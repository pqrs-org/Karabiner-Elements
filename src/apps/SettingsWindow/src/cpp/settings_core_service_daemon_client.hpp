#pragma once

#include "core_service_daemon_client.hpp"
#include "settings.hpp"
#include "settings_callback_manager.hpp"

class settings_core_service_daemon_client final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  settings_core_service_daemon_client(const settings_core_service_daemon_client&) = delete;

  settings_core_service_daemon_client()
      : dispatcher_client(),
        core_service_daemon_client_(std::make_unique<krbn::core_service_daemon_client>()) {
    core_service_daemon_client_->received.connect([this](auto&& operation_type,
                                                         auto&& json) {
      try {
        switch (operation_type) {
          case krbn::operation_type::connected_devices: {
            auto json_dump = krbn::json_utility::dump(json.at("connected_devices"));

            for (const auto& callback : connected_devices_received_callback_manager_.get_callbacks()) {
              callback(json_dump.c_str());
            }
            break;
          }

          case krbn::operation_type::system_variables: {
            auto json_dump = krbn::json_utility::dump(json.at("system_variables"));

            for (const auto& callback : system_variables_received_callback_manager_.get_callbacks()) {
              callback(json_dump.c_str());
            }
            break;
          }

          default:
            break;
        }
      } catch (const std::exception&) {
        krbn::logger::get_logger()->error("settings_core_service_daemon_client received data is corrupted");
      }
    });
  }

  ~settings_core_service_daemon_client() override {
    detach_from_dispatcher([this] {
      core_service_daemon_client_ = nullptr;
    });
  }

  void async_start() const {
    core_service_daemon_client_->async_start();
  }

  void async_get_connected_devices() const {
    core_service_daemon_client_->async_get_connected_devices();
  }

  void async_get_system_variables() const {
    core_service_daemon_client_->async_get_system_variables();
  }

  void async_set_app_icon(int number) const {
    core_service_daemon_client_->async_set_app_icon(number);
  }

  void register_connected_devices_received_callback(krbn_core_service_daemon_client_connected_devices_received_t callback) {
    enqueue_to_dispatcher([this, callback] {
      connected_devices_received_callback_manager_.register_callback(callback);
    });
  }

  void register_system_variables_received_callback(krbn_core_service_daemon_client_system_variables_received_t callback) {
    enqueue_to_dispatcher([this, callback] {
      system_variables_received_callback_manager_.register_callback(callback);
    });
  }

private:
  std::unique_ptr<krbn::core_service_daemon_client> core_service_daemon_client_;
  settings_callback_manager<krbn_core_service_daemon_client_connected_devices_received_t> connected_devices_received_callback_manager_;
  settings_callback_manager<krbn_core_service_daemon_client_system_variables_received_t> system_variables_received_callback_manager_;
};
