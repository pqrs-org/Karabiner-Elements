#pragma once

// `krbn::console_user_server::components_manager` can be used safely in a multi-threaded environment.

#include "app_icon.hpp"
#include "application_launcher.hpp"
#include "console_user_id_changed_client.hpp"
#include "console_user_server/ui_bridge.hpp"
#include "constants.hpp"
#include "core_service_daemon_client.hpp"
#include "logger.hpp"
#include "monitor/configuration_monitor.hpp"
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
#include <unistd.h>

namespace krbn::console_user_server {
class components_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  components_manager(const components_manager&) = delete;

  components_manager(std::shared_ptr<ui_bridge> ui_bridge)
      : dispatcher_client(),
        console_user_id_changed_client_(std::make_shared<console_user_id_changed_client>()),
        session_monitor_(std::make_unique<pqrs::osx::session::monitor>(weak_dispatcher_)),
        configuration_monitor_(std::make_unique<configuration_monitor>(constants::get_user_core_configuration_file_path().string(),
                                                                       geteuid(),
                                                                       core_configuration::error_handling::loose)),
        settings_window_guidance_manager_dispatcher_time_source_(std::make_shared<pqrs::dispatcher::hardware_time_source>()),
        settings_window_guidance_manager_dispatcher_(std::make_shared<pqrs::dispatcher::dispatcher>(settings_window_guidance_manager_dispatcher_time_source_)),
        settings_window_guidance_manager_(std::make_shared<settings_window_guidance_manager>(settings_window_guidance_manager_dispatcher_,
                                                                                             settings_window_guidance_manager::make_default_guidance_context_maker())),
        software_function_handler_(std::make_shared<software_function_handler>()),
        ui_bridge_(std::move(ui_bridge)) {
    configuration_monitor_->core_configuration_updated.connect([this](auto&& weak_core_configuration) {
      if (auto core_configuration = weak_core_configuration.lock()) {
        core_configuration_ = core_configuration;

        if (core_configuration->get_machine_specific().get_entry().get_enable_multitouch_extension()) {
          services_utility::register_multitouch_extension_agent();
        } else {
          services_utility::unregister_multitouch_extension_agent();
        }

        publish_ui_state(core_configuration.get());
      }
    });

    configuration_monitor_->load_state_changed.connect([this](auto load_state) {
      settings_window_guidance_manager_->async_update_console_user_server_karabiner_json_permission_error(
          load_state == core_configuration::core_configuration::load_state::permission_error);

      if (load_state != core_configuration::core_configuration::load_state::loaded) {
        core_configuration_ = nullptr;
        publish_ui_state(nullptr);
      }
    });

    select_profile_connection_ = ui_bridge_->profile_selection_requested.connect([this](auto index) {
      if (core_configuration_) {
        core_configuration_->select_profile(index);
        core_configuration_->sync_save_to_file();
        publish_ui_state(core_configuration_.get());
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

    console_user_id_changed_client_->connect_failed.connect([](auto&&) {
      // Do nothing
    });

    console_user_id_changed_client_->closed.connect([] {
      // Do nothing
    });

    console_user_id_changed_client_->core_service_daemon_server_bound.connect([this](auto&& uid) {
      if (on_console_ == std::optional<bool>(true) &&
          uid == std::optional<uid_t>(getuid())) {
        start_core_service_daemon_client();
      }
    });

    //
    // session_monitor_
    //

    session_monitor_->on_console_changed.connect([this](auto&& on_console) {
      logger::get_logger()->debug("on_console_changed: on_console:{}", on_console);

      on_console_ = on_console;

      stop_core_service_daemon_client();

      console_user_id_changed_client_->async_console_user_id_changed(on_console);

      // Delay start_core_service_daemon_client until core_service_daemon_server_bound is received.
      // The core service daemon recreates its receiver socket for the new console user,
      // so connecting before the socket ownership and permissions are updated may fail.
    });
  }

  ~components_manager() override {
    detach_from_dispatcher([this] {
      stop_core_service_daemon_client();
      publish_ui_state(nullptr);

      receiver_ = nullptr;
      software_function_handler_ = nullptr;
      settings_window_guidance_manager_ = nullptr;
      settings_window_guidance_manager_dispatcher_ = nullptr;
      settings_window_guidance_manager_dispatcher_time_source_ = nullptr;
      session_monitor_ = nullptr;
      select_profile_connection_.disconnect();
      configuration_monitor_ = nullptr;
      core_configuration_ = nullptr;
      console_user_id_changed_client_ = nullptr;
    });
  }

  void async_start() {
    enqueue_to_dispatcher([this] {
      console_user_id_changed_client_->async_start();
      session_monitor_->async_start(std::chrono::milliseconds(1000));
      configuration_monitor_->async_start();
      settings_window_guidance_manager_->async_start();
      receiver_ = std::make_unique<receiver>(settings_window_guidance_manager_,
                                             software_function_handler_);
    });
  }

private:
  void publish_ui_state(const core_configuration::core_configuration* configuration) const {
    const auto configuration_loaded = configuration != nullptr;
    const core_configuration::details::global_configuration default_global_configuration(
        nlohmann::json::object(),
        core_configuration::error_handling::loose);
    const auto& global_configuration = configuration_loaded
                                           ? configuration->get_global_configuration()
                                           : default_global_configuration;

    nlohmann::json profiles = nlohmann::json::array();
    if (configuration) {
      size_t index = 0;
      for (const auto& profile : configuration->get_profiles()) {
        profiles.push_back({
            {"id", index},
            {"name", profile->get_name()},
            {"selected", profile->get_selected()},
        });
        ++index;
      }
    }

    ui_bridge_->set_ui_state(
        nlohmann::json(
            {
                {
                    "configurationLoaded",
                    configuration_loaded,
                },
                {
                    "appIconNumber",
                    app_icon(constants::get_system_app_icon_configuration_file_path()).get_number(),
                },
                {
                    "menuSettings",
                    {
                        {"showIcon", configuration_loaded && global_configuration.get_show_in_menu_bar()},
                        {"showProfileName", configuration_loaded && global_configuration.get_show_profile_name_in_menu_bar()},
                        {"showAdditionalMenuItems", configuration_loaded && global_configuration.get_show_additional_menu_items()},
                        {"enableMultitouchExtension", configuration && configuration->get_machine_specific().get_entry().get_enable_multitouch_extension()},
                    },
                },
                {
                    "notificationWindowSettings",
                    {
                        {"enabled", configuration_loaded && global_configuration.get_enable_notification_window()},
                        {"position", global_configuration.get_notification_window_position()},
                        {"respectScreenVisibleFrame", global_configuration.get_notification_window_respect_screen_visible_frame()},
                        {"showIcon", global_configuration.get_notification_window_show_icon()},
                        {"fontSize", global_configuration.get_notification_window_font_size()},
                        {"colors",
                         {
                             {"light",
                              {
                                  {"backgroundColor", global_configuration.get_notification_window_colors().get_light().get_background_color()},
                                  {"textColor", global_configuration.get_notification_window_colors().get_light().get_text_color()},
                              }},
                             {"dark",
                              {
                                  {"backgroundColor", global_configuration.get_notification_window_colors().get_dark().get_background_color()},
                                  {"textColor", global_configuration.get_notification_window_colors().get_dark().get_text_color()},
                              }},
                         }},
                    },
                },
                {
                    "profiles",
                    profiles,
                },
            })
            .dump());
  }

  void start_core_service_daemon_client() {
    if (core_service_daemon_client_) {
      return;
    }

    if (on_console_ != std::optional<bool>(true)) {
      return;
    }

    core_service_daemon_client_ = std::make_shared<core_service_daemon_client>();

    core_service_daemon_client_->connected.connect([this] {
      core_service_daemon_client_->async_start_device_grabber(constants::get_user_core_configuration_file_path());
      core_service_daemon_client_->async_observe_notification_message();

      stop_child_components();
      start_child_components();
    });

    core_service_daemon_client_->connect_failed.connect([this](auto&&) {
      stop_child_components();
    });

    core_service_daemon_client_->closed.connect([this] {
      stop_child_components();
    });

    core_service_daemon_client_->received.connect([this](auto&& operation_type,
                                                         auto&& json) {
      if (operation_type == krbn::operation_type::app_icon_changed) {
        publish_ui_state(core_configuration_.get());
        return;
      }

      if (operation_type == krbn::operation_type::notification_message) {
        ui_bridge_->set_notification_message(json.at("notification_message").template get<std::string>());
        return;
      }

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

  //
  // Core components
  //

  std::optional<bool> on_console_;
  std::shared_ptr<console_user_id_changed_client> console_user_id_changed_client_;
  std::unique_ptr<pqrs::osx::session::monitor> session_monitor_;
  std::unique_ptr<configuration_monitor> configuration_monitor_;
  std::shared_ptr<core_configuration::core_configuration> core_configuration_;

  // settings_window_guidance_manager internally calls services_utility::core_daemons_enabled() and similar functions.
  // These are expensive operations that launch processes, and using the same dispatcher as receiver would block receiver processing.
  // Therefore, a dedicated dispatcher is required for settings_window_guidance_manager.
  std::shared_ptr<pqrs::dispatcher::hardware_time_source> settings_window_guidance_manager_dispatcher_time_source_;
  std::shared_ptr<pqrs::dispatcher::dispatcher> settings_window_guidance_manager_dispatcher_;
  std::shared_ptr<settings_window_guidance_manager> settings_window_guidance_manager_;

  std::shared_ptr<software_function_handler> software_function_handler_;
  std::shared_ptr<core_service_daemon_client> core_service_daemon_client_;

  //
  // Child components
  //

  std::unique_ptr<pqrs::osx::system_preferences_monitor> system_preferences_monitor_;
  std::unique_ptr<pqrs::osx::input_source_monitor> input_source_monitor_;
  std::unique_ptr<receiver> receiver_;

  //
  // For UI (menu, notification window)
  //

  std::shared_ptr<ui_bridge> ui_bridge_;
  // Declare this last so that it is disconnected before the other members are destroyed.
  nod::scoped_connection select_profile_connection_;
};
} // namespace krbn::console_user_server
