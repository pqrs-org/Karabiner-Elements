#pragma once

#include "core_service_daemon_client.hpp"
#include "settings.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <utility>

class settings_core_service_daemon_client final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  settings_core_service_daemon_client(const settings_core_service_daemon_client&) = delete;

  settings_core_service_daemon_client(std::function<void(const krbn::connected_devices&)> connected_devices_updated_callback,
                                      krbn_core_service_daemon_client_connected_devices_received_t connected_devices_received_callback,
                                      krbn_core_service_daemon_client_system_variables_received_t system_variables_received_callback)
      : dispatcher_client(),
        connected_devices_updated_callback_(std::move(connected_devices_updated_callback)),
        connected_devices_received_callback_(connected_devices_received_callback),
        system_variables_received_callback_(system_variables_received_callback) {
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
            auto connected_devices_json = json.at("connected_devices");
            auto connected_devices = connected_devices_json.template get<krbn::connected_devices>();

            connected_devices_updated_callback_(connected_devices);

            const auto& devices = connected_devices.get_devices();
            for (std::size_t i = 0; i < devices.size(); ++i) {
              connected_devices_json[i]["device_identifiers_json_string"] =
                  devices[i]->get_device_identifiers().to_normalized_json().dump();
            }

            auto json_dump = krbn::json_utility::dump(connected_devices_json);

            connected_devices_received_callback_(json_dump.c_str());
            break;
          }

          case krbn::operation_type::system_variables: {
            auto json_dump = krbn::json_utility::dump(json.at("system_variables"));

            system_variables_received_callback_(json_dump.c_str());
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

private:
  const std::function<void(const krbn::connected_devices&)> connected_devices_updated_callback_;
  std::shared_ptr<krbn::core_service_daemon_client> core_service_daemon_client_;
  const krbn_core_service_daemon_client_connected_devices_received_t connected_devices_received_callback_;
  const krbn_core_service_daemon_client_system_variables_received_t system_variables_received_callback_;
};
