#pragma once

// `krbn::core_service_daemon_client` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "types.hpp"
#include "xpc_client.hpp"
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <pqrs/osx/iokit_types.hpp>
#include <pqrs/osx/system_preferences.hpp>
#include <pqrs/osx/system_preferences/extra/nlohmann_json.hpp>
#include <utility>

namespace krbn {
class core_service_daemon_client final : public xpc_client {
public:
  core_service_daemon_client(const core_service_daemon_client&) = delete;

  core_service_daemon_client()
      : xpc_client({
            .client_name = "core_service_daemon_client",
            .service_name = constants::get_core_service_daemon_mach_service_name(),
            .signing_identifier = "org.pqrs.Karabiner-Core-Service",
            .privileged = true,
        }) {
  }

  // console_user_server -> core_service (daemon)
  void async_console_user_id_changed(bool on_console) const {
    enqueue_to_dispatcher([this, on_console] {
      nlohmann::json json{
          {"operation_type", operation_type::console_user_id_changed},
          {"on_console", on_console},
      };

      async_request(std::move(json));
    });
  }

  void async_system_preferences_updated(std::shared_ptr<pqrs::osx::system_preferences::properties> properties) const {
    enqueue_to_dispatcher([this, properties] {
      if (properties) {
        nlohmann::json json{
            {"operation_type", operation_type::system_preferences_updated},
            {"system_preferences_properties", *properties},
        };

        async_request(std::move(json));
      }
    });
  }

  void async_core_service_bundle_permission_check_result(bool iohid_listen_event_allowed,
                                                         bool accessibility_process_trusted) const {
    enqueue_to_dispatcher([this, iohid_listen_event_allowed, accessibility_process_trusted] {
      nlohmann::json json{
          {"operation_type", operation_type::core_service_bundle_permission_check_result},
          {"iohid_listen_event_allowed", iohid_listen_event_allowed},
          {"accessibility_process_trusted", accessibility_process_trusted},
      };

      async_request(std::move(json));
    });
  }

  void async_frontmost_application_changed(const application& application) const {
    enqueue_to_dispatcher([this, application] {
      nlohmann::json json{
          {"operation_type", operation_type::frontmost_application_changed},
          {"frontmost_application", application},
      };

      async_request(std::move(json));
    });
  }

  void async_focused_ui_element_changed(const focused_ui_element& focused_ui_element) const {
    enqueue_to_dispatcher([this, focused_ui_element] {
      nlohmann::json json{
          {"operation_type", operation_type::focused_ui_element_changed},
          {"focused_ui_element", focused_ui_element},
      };

      async_request(std::move(json));
    });
  }

  void async_input_source_changed(std::shared_ptr<pqrs::osx::input_source::properties> properties) const {
    enqueue_to_dispatcher([this, properties] {
      if (properties) {
        nlohmann::json json{
            {"operation_type", operation_type::input_source_changed},
            {"input_source_properties", *properties},
        };

        async_request(std::move(json));
      }
    });
  }

  void async_start_device_grabber(const std::filesystem::path& user_core_configuration_file_path) const {
    enqueue_to_dispatcher([this, user_core_configuration_file_path] {
      nlohmann::json json{
          {"operation_type", operation_type::start_device_grabber},
          {"user_core_configuration_file_path", user_core_configuration_file_path},
      };

      async_request(std::move(json));
    });
  }

  void async_temporarily_ignore_device(std::optional<device_id> device_id) const {
    enqueue_to_dispatcher([this, device_id] {
      nlohmann::json json{
          {"operation_type", operation_type::temporarily_ignore_device},
          {"device_id", device_id ? nlohmann::json(type_safe::get(*device_id)) : nlohmann::json(nullptr)},
      };

      async_request(std::move(json));
    });
  }

  void async_get_manipulator_environment() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::get_manipulator_environment},
      };

      async_request(std::move(json));
    });
  }

  void async_observe_connected_devices() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::observe_connected_devices},
      };

      async_request(std::move(json));
    });
  }

  void async_observe_notification_message() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::observe_notification_message},
      };

      async_request(std::move(json));
    });
  }

  void async_get_system_variables() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::get_system_variables},
      };

      async_request(std::move(json));
    });
  }

  void async_get_multitouch_extension_variables() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::get_multitouch_extension_variables},
      };

      async_request(std::move(json));
    });
  }

  void async_connect_multitouch_extension() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::connect_multitouch_extension},
      };

      async_request(std::move(json));
    });
  }

  void async_set_app_icon(int number) const {
    enqueue_to_dispatcher([this, number] {
      nlohmann::json json{
          {"operation_type", operation_type::set_app_icon},
          {"number", number},
      };

      async_request(std::move(json));
    });
  }

  void async_set_variables(const nlohmann::json& variables) const {
    async_set_variables_with_completion_handler(variables,
                                                nullptr);
  }

  void async_set_variables_with_completion_handler(
      const nlohmann::json& variables,
      std::function<void(const std::error_code&)> completion_handler) const {
    enqueue_to_dispatcher([this, variables, completion_handler = std::move(completion_handler)] {
      nlohmann::json json{
          {"operation_type", operation_type::set_variables},
          {"variables", variables},
      };

      async_request(std::move(json),
                    std::move(completion_handler));
    });
  }

  void async_clear_user_variables() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::clear_user_variables},
      };

      async_request(std::move(json));
    });
  }
};
} // namespace krbn
