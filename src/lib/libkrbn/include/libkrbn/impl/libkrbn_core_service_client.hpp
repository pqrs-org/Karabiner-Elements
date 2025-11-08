#pragma once

#include "core_service_client.hpp"
#include "libkrbn/libkrbn.h"
#include "libkrbn_callback_manager.hpp"

class libkrbn_core_service_client final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  libkrbn_core_service_client(const libkrbn_core_service_client&) = delete;

  libkrbn_core_service_client(std::optional<std::string> client_socket_directory_name)
      : dispatcher_client(),
        status_(libkrbn_core_service_client_status_none) {
    core_service_client_ = std::make_unique<krbn::core_service_client>(client_socket_directory_name);

    core_service_client_->connected.connect([this] {
      set_status(libkrbn_core_service_client_status_connected);
    });

    core_service_client_->connect_failed.connect([this](auto&& error_code) {
      set_status(libkrbn_core_service_client_status_connect_failed);
    });

    core_service_client_->closed.connect([this] {
      set_status(libkrbn_core_service_client_status_closed);
    });

    core_service_client_->received.connect([this](auto&& buffer,
                                                  auto&& sender_endpoint) {
      if (buffer) {
        if (buffer->empty()) {
          return;
        }

        try {
          nlohmann::json json = nlohmann::json::from_msgpack(*buffer);
          switch (json.at("operation_type").get<krbn::operation_type>()) {
            case krbn::operation_type::manipulator_environment: {
              auto json_dump = krbn::json_utility::dump(json.at("manipulator_environment"));

              for (const auto& c : manipulator_environment_received_callback_manager_.get_callbacks()) {
                c(json_dump.c_str());
              }

              break;
            }

            case krbn::operation_type::system_variables: {
              auto json_dump = krbn::json_utility::dump(json.at("system_variables"));

              for (const auto& c : system_variables_received_callback_manager_.get_callbacks()) {
                c(json_dump.c_str());
              }

              break;
            }

            default:
              break;
          }
        } catch (std::exception& e) {
          krbn::logger::get_logger()->error("libkrbn_core_service_client received data is corrupted");
        }
      }
    });
  }

  ~libkrbn_core_service_client(void) {
    detach_from_dispatcher([this] {
      core_service_client_ = nullptr;
    });
  }

  void async_start(void) const {
    core_service_client_->async_start();
  }

  libkrbn_core_service_client_status get_status(void) const {
    return status_;
  }

  void async_temporarily_ignore_all_devices(bool value) {
    core_service_client_->async_temporarily_ignore_all_devices(value);
  }

  void async_get_manipulator_environment(void) {
    core_service_client_->async_get_manipulator_environment();
  }

  void async_get_system_variables(void) {
    core_service_client_->async_get_system_variables();
  }

  void async_connect_multitouch_extension(void) {
    core_service_client_->async_connect_multitouch_extension();
  }

  void async_set_app_icon(int number) const {
    core_service_client_->async_set_app_icon(number);
  }

  void async_set_variable(const std::string& name, int value) const {
    auto json = nlohmann::json::object({
        {name, value},
    });
    core_service_client_->async_set_variables(json);
  }

  void sync_set_variable(const std::string& name, int value) const {
    auto wait = pqrs::make_thread_wait();

    auto json = nlohmann::json::object({
        {name, value},
    });
    core_service_client_->async_set_variables(json, [wait] {
      wait->notify();
    });

    wait->wait_notice();
  }

  void register_libkrbn_core_service_client_status_changed_callback(libkrbn_core_service_client_status_changed_t callback) {
    enqueue_to_dispatcher([this, callback] {
      status_changed_callback_manager_.register_callback(callback);
    });
  }

  void unregister_libkrbn_core_service_client_status_changed_callback(libkrbn_core_service_client_status_changed_t callback) {
    enqueue_to_dispatcher([this, callback] {
      status_changed_callback_manager_.unregister_callback(callback);
    });
  }

  void register_libkrbn_core_service_client_manipulator_environment_received_callback(libkrbn_core_service_client_manipulator_environment_received_t callback) {
    enqueue_to_dispatcher([this, callback] {
      manipulator_environment_received_callback_manager_.register_callback(callback);
    });
  }

  void unregister_libkrbn_core_service_client_manipulator_environment_received_callback(libkrbn_core_service_client_manipulator_environment_received_t callback) {
    enqueue_to_dispatcher([this, callback] {
      manipulator_environment_received_callback_manager_.unregister_callback(callback);
    });
  }

  void register_libkrbn_core_service_client_system_variables_received_callback(libkrbn_core_service_client_manipulator_environment_received_t callback) {
    enqueue_to_dispatcher([this, callback] {
      system_variables_received_callback_manager_.register_callback(callback);
    });
  }

  void unregister_libkrbn_core_service_client_system_variables_received_callback(libkrbn_core_service_client_manipulator_environment_received_t callback) {
    enqueue_to_dispatcher([this, callback] {
      system_variables_received_callback_manager_.unregister_callback(callback);
    });
  }

private:
  // This method should be called in the shared dispatcher thread.
  void set_status(libkrbn_core_service_client_status status) {
    status_ = status;

    for (const auto& c : status_changed_callback_manager_.get_callbacks()) {
      c();
    }
  }

  std::unique_ptr<krbn::core_service_client> core_service_client_;
  libkrbn_core_service_client_status status_;
  libkrbn_callback_manager<libkrbn_core_service_client_status_changed_t> status_changed_callback_manager_;
  libkrbn_callback_manager<libkrbn_core_service_client_manipulator_environment_received_t> manipulator_environment_received_callback_manager_;
  libkrbn_callback_manager<libkrbn_core_service_client_system_variables_received_t> system_variables_received_callback_manager_;
};
