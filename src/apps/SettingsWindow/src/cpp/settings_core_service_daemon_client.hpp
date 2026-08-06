#pragma once

#include "core_service_daemon_client.hpp"
#include "settings.hpp"
#include <atomic>
#include <memory>

class settings_core_service_daemon_client final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  settings_core_service_daemon_client(const settings_core_service_daemon_client&) = delete;

  settings_core_service_daemon_client()
      : dispatcher_client() {
    start();
  }

  ~settings_core_service_daemon_client() override {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  void start() {
    if (std::atomic_load(&core_service_daemon_client_)) {
      return;
    }

    auto client = std::make_shared<krbn::core_service_daemon_client>();

    client->received.connect([this](auto&& operation_type,
                                    auto&& json) {
      try {
        switch (operation_type) {
          case krbn::operation_type::connected_devices: {
            auto json_dump = krbn::json_utility::dump(json.at("connected_devices"));

            if (auto callback = connected_devices_received_callback_.load()) {
              callback(json_dump.c_str());
            }
            break;
          }

          case krbn::operation_type::system_variables: {
            auto json_dump = krbn::json_utility::dump(json.at("system_variables"));

            if (auto callback = system_variables_received_callback_.load()) {
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

    std::atomic_store(&core_service_daemon_client_, client);
  }

  void stop() {
    std::atomic_store(&core_service_daemon_client_,
                      std::shared_ptr<krbn::core_service_daemon_client>());
  }

  void async_start() const {
    if (auto client = std::atomic_load(&core_service_daemon_client_)) {
      client->async_start();
    }
  }

  void async_get_connected_devices() const {
    if (auto client = std::atomic_load(&core_service_daemon_client_)) {
      client->async_get_connected_devices();
    }
  }

  void async_get_system_variables() const {
    if (auto client = std::atomic_load(&core_service_daemon_client_)) {
      client->async_get_system_variables();
    }
  }

  void async_set_app_icon(int number) const {
    if (auto client = std::atomic_load(&core_service_daemon_client_)) {
      client->async_set_app_icon(number);
    }
  }

  void set_connected_devices_received_callback(krbn_core_service_daemon_client_connected_devices_received_t callback) {
    connected_devices_received_callback_.store(callback);
  }

  void set_system_variables_received_callback(krbn_core_service_daemon_client_system_variables_received_t callback) {
    system_variables_received_callback_.store(callback);
  }

private:
  std::shared_ptr<krbn::core_service_daemon_client> core_service_daemon_client_;
  std::atomic<krbn_core_service_daemon_client_connected_devices_received_t> connected_devices_received_callback_{nullptr};
  std::atomic<krbn_core_service_daemon_client_system_variables_received_t> system_variables_received_callback_{nullptr};
};
