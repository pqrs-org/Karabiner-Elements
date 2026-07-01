#pragma once

// `krbn::console_user_server::components_manager` can be used safely in a multi-threaded environment.

#include "application_launcher.hpp"
#include "components_manager_killer.hpp"
#include "console_user_id_changed_client.hpp"
#include "constants.hpp"
#include "core_service_daemon_client.hpp"
#include "logger.hpp"
#include "monitor/version_monitor.hpp"
#include "receiver.hpp"
#include "services_utility.hpp"
#include "settings_window_guidance_manager.hpp"
#include "software_function_handler.hpp"
#include <filesystem>
#include <optional>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/input_source_monitor.hpp>
#include <pqrs/osx/json_file_monitor.hpp>
#include <pqrs/osx/session.hpp>
#include <pqrs/osx/system_preferences_monitor.hpp>
#include <sys/stat.h>
#include <thread>

namespace krbn::console_user_server {
class components_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  components_manager(const components_manager&) = delete;

  components_manager()
      : dispatcher_client(),
        version_monitor_(std::make_unique<krbn::version_monitor>(krbn::constants::get_version_file_path())),
        console_user_id_changed_client_(std::make_shared<console_user_id_changed_client>()),
        session_monitor_(std::make_unique<pqrs::osx::session::monitor>(weak_dispatcher_)),
        settings_window_guidance_manager_dispatcher_time_source_(std::make_shared<pqrs::dispatcher::hardware_time_source>()),
        settings_window_guidance_manager_dispatcher_(std::make_shared<pqrs::dispatcher::dispatcher>(settings_window_guidance_manager_dispatcher_time_source_)),
        settings_window_guidance_manager_(std::make_shared<settings_window_guidance_manager>(settings_window_guidance_manager_dispatcher_,
                                                                                             settings_window_guidance_manager::make_default_guidance_context_maker())),
        software_function_handler_(std::make_shared<software_function_handler>()) {
    //
    // version_monitor_
    //

    version_monitor_->changed.connect([](auto&& version) {
      if (auto killer = components_manager_killer::get_shared_components_manager_killer()) {
        killer->async_kill();
      }
    });

    //
    // console_user_id_changed_client_
    //

    console_user_id_changed_client_->connected.connect([this] {
      if (on_console_) {
        console_user_id_changed_client_->async_console_user_id_changed(*on_console_);
      }
    });

    //
    // session_monitor_
    //

    session_monitor_->on_console_changed.connect([this](auto&& on_console) {
      logger::get_logger()->info("on_console_changed: on_console:{}", on_console);

      on_console_ = on_console;

      console_user_id_changed_client_->async_console_user_id_changed(on_console);

      version_monitor_->async_manual_check();

      stop_core_service_daemon_client();
      start_core_service_daemon_client();
    });
  }

  ~components_manager() override {
    detach_from_dispatcher([this] {
      stop_core_service_daemon_client();

      receiver_ = nullptr;
      software_function_handler_ = nullptr;
      settings_window_guidance_manager_ = nullptr;
      settings_window_guidance_manager_dispatcher_ = nullptr;
      settings_window_guidance_manager_dispatcher_time_source_ = nullptr;
      session_monitor_ = nullptr;
      console_user_id_changed_client_ = nullptr;
      version_monitor_ = nullptr;
    });
  }

  void async_start() {
    enqueue_to_dispatcher([this] {
      version_monitor_->async_start();
      console_user_id_changed_client_->async_start();
      session_monitor_->async_start(std::chrono::milliseconds(1000));
      settings_window_guidance_manager_->async_start();
      receiver_ = std::make_unique<receiver>(settings_window_guidance_manager_,
                                             software_function_handler_);
    });
  }

private:
  void start_core_service_daemon_client() {
    if (core_service_daemon_client_) {
      return;
    }

    if (on_console_ != std::optional<bool>(true)) {
      return;
    }

    core_service_daemon_client_ = std::make_shared<core_service_daemon_client>();

    core_service_daemon_client_->connected.connect([this] {
      version_monitor_->async_manual_check();

      core_service_daemon_client_->async_start_device_grabber(constants::get_user_core_configuration_file_path());

      stop_child_components();
      start_child_components();
    });

    core_service_daemon_client_->connect_failed.connect([this](auto&& error_code) {
      version_monitor_->async_manual_check();

      stop_child_components();
    });

    core_service_daemon_client_->closed.connect([this] {
      version_monitor_->async_manual_check();

      stop_child_components();
    });

    core_service_daemon_client_->received.connect([this](auto&& operation_type,
                                                         auto&& json) {
      if (receiver_) {
        receiver_->handle_core_service_daemon_message(operation_type,
                                                      json);
      }
    });

    core_service_daemon_client_->async_start();
  }

  void stop_core_service_daemon_client() {
    core_service_daemon_client_ = nullptr;
    stop_child_components();
  }

  void start_child_components() {
    // system_preferences_monitor_

    system_preferences_monitor_ = std::make_unique<pqrs::osx::system_preferences_monitor>(weak_dispatcher_);

    system_preferences_monitor_->system_preferences_changed.connect([this](auto&& properties_ptr) {
      if (core_service_daemon_client_) {
        core_service_daemon_client_->async_system_preferences_updated(properties_ptr);
      }
    });

    system_preferences_monitor_->async_start(std::chrono::milliseconds(3000));

    // input_source_monitor_

    input_source_monitor_ = std::make_unique<pqrs::osx::input_source_monitor>(
        pqrs::dispatcher::extra::get_shared_dispatcher());

    input_source_monitor_->input_source_changed.connect([this](auto&& input_source_ptr) {
      if (input_source_ptr && core_service_daemon_client_) {
        auto properties = std::make_shared<pqrs::osx::input_source::properties>(*input_source_ptr);
        core_service_daemon_client_->async_input_source_changed(properties);
      }
    });

    input_source_monitor_->async_start();
  }

  void stop_child_components() {
    system_preferences_monitor_ = nullptr;
    input_source_monitor_ = nullptr;
  }

  // Core components

  std::unique_ptr<version_monitor> version_monitor_;

  std::optional<bool> on_console_;
  std::shared_ptr<console_user_id_changed_client> console_user_id_changed_client_;
  std::unique_ptr<pqrs::osx::session::monitor> session_monitor_;

  // settings_window_guidance_manager internally calls services_utility::core_daemons_enabled() and similar functions.
  // These are expensive operations that launch processes, and using the same dispatcher as receiver would block receiver processing.
  // Therefore, a dedicated dispatcher is required for settings_window_guidance_manager.
  std::shared_ptr<pqrs::dispatcher::hardware_time_source> settings_window_guidance_manager_dispatcher_time_source_;
  std::shared_ptr<pqrs::dispatcher::dispatcher> settings_window_guidance_manager_dispatcher_;
  std::shared_ptr<settings_window_guidance_manager> settings_window_guidance_manager_;

  std::shared_ptr<software_function_handler> software_function_handler_;
  std::shared_ptr<core_service_daemon_client> core_service_daemon_client_;

  // Child components

  std::unique_ptr<pqrs::osx::system_preferences_monitor> system_preferences_monitor_;
  std::unique_ptr<pqrs::osx::input_source_monitor> input_source_monitor_;
  std::unique_ptr<receiver> receiver_;
};
} // namespace krbn::console_user_server
