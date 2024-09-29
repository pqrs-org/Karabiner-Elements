#pragma once

// `krbn::console_user_server::components_manager` can be used safely in a multi-threaded environment.

#include "application_launcher.hpp"
#include "chrono_utility.hpp"
#include "components_manager_killer.hpp"
#include "constants.hpp"
#include "filesystem_utility.hpp"
#include "grabber_client.hpp"
#include "logger.hpp"
#include "monitor/configuration_monitor.hpp"
#include "monitor/version_monitor.hpp"
#include "services_utility.hpp"
#include "shell_command_handler.hpp"
#include "software_function_handler.hpp"
#include "updater_process_manager.hpp"
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/frontmost_application_monitor.hpp>
#include <pqrs/osx/input_source_monitor.hpp>
#include <pqrs/osx/input_source_selector.hpp>
#include <pqrs/osx/input_source_selector/extra/nlohmann_json.hpp>
#include <pqrs/osx/json_file_monitor.hpp>
#include <pqrs/osx/session.hpp>
#include <pqrs/osx/system_preferences_monitor.hpp>
#include <thread>

namespace krbn {
namespace console_user_server {
class components_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  components_manager(const components_manager&) = delete;

  components_manager(void) : dispatcher_client() {
    //
    // version_monitor_
    //

    version_monitor_ = std::make_unique<krbn::version_monitor>(krbn::constants::get_version_file_path());

    version_monitor_->changed.connect([](auto&& version) {
      if (auto killer = components_manager_killer::get_shared_components_manager_killer()) {
        killer->async_kill();
      }
    });

    //
    // session_monitor_
    //

    session_monitor_ = std::make_unique<pqrs::osx::session::monitor>(weak_dispatcher_);

    session_monitor_->on_console_changed.connect([this](auto&& on_console) {
      logger::get_logger()->info("on_console_changed: on_console:{}", on_console);

      on_console_ = on_console;

      version_monitor_->async_manual_check();

      pqrs::filesystem::create_directory_with_intermediate_directories(
          constants::get_user_configuration_directory(),
          0700);

      stop_grabber_client();
      start_grabber_client();
    });
  }

  virtual ~components_manager(void) {
    detach_from_dispatcher([this] {
      stop_grabber_client();

      session_monitor_ = nullptr;
      version_monitor_ = nullptr;
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      version_monitor_->async_start();
      session_monitor_->async_start(std::chrono::milliseconds(1000));
    });
  }

private:
  void start_grabber_client(void) {
    if (grabber_client_) {
      return;
    }

    if (on_console_ != true) {
      return;
    }

    // Note:
    // The socket file path length must be <= 103 because sizeof(sockaddr_un.sun_path) == 104.
    // So we use the shorten name console_user_server_grabber_client -> con_usr_srv_grb_clnt.
    //
    // Example:
    // `/Library/Application Support/org.pqrs/tmp/user/501/con_usr_srv_grb_clnt/17d502ed0dd6f828.sock`

    grabber_client_ = std::make_shared<grabber_client>("con_usr_srv_grb_clnt");

    grabber_client_->connected.connect([this] {
      version_monitor_->async_manual_check();

      if (grabber_client_) {
        grabber_client_->async_connect_console_user_server();
      }

      stop_child_components();
      start_child_components();
    });

    grabber_client_->connect_failed.connect([this](auto&& error_code) {
      version_monitor_->async_manual_check();

      stop_child_components();
    });

    grabber_client_->closed.connect([this] {
      version_monitor_->async_manual_check();

      stop_child_components();
    });

    grabber_client_->received.connect([this](auto&& buffer, auto&& sender_endpoint) {
      if (buffer) {
        if (buffer->empty()) {
          return;
        }

        try {
          nlohmann::json json = nlohmann::json::from_msgpack(*buffer);
          switch (json.at("operation_type").get<operation_type>()) {
            case operation_type::select_input_source: {
              using specifiers_t = std::vector<pqrs::osx::input_source_selector::specifier>;
              auto specifiers = json.at("input_source_specifiers").get<specifiers_t>();
              if (input_source_selector_) {
                input_source_selector_->async_select(std::make_shared<specifiers_t>(specifiers));
              }
              break;
            }

            case operation_type::shell_command_execution: {
              auto shell_command = json.at("shell_command").get<std::string>();
              if (shell_command_handler_) {
                shell_command_handler_->run(shell_command);
              }
              break;
            }

            case operation_type::software_function: {
              if (software_function_handler_) {
                software_function_handler_->execute_software_function(
                    json.at("software_function").get<software_function>());
              }
              break;
            }

            default:
              break;
          }
          return;
        } catch (std::exception& e) {
          logger::get_logger()->error("received data is corrupted");
        }
      }
    });

    grabber_client_->next_heartbeat_deadline_exceeded.connect([this] {
      stop_grabber_client();
      start_grabber_client();
    });

    grabber_client_->async_start();
  }

  void stop_grabber_client(void) {
    grabber_client_ = nullptr;
    stop_child_components();
  }

  void start_child_components(void) {
    configuration_monitor_ = std::make_shared<configuration_monitor>(constants::get_user_core_configuration_file_path(),
                                                                     geteuid(),
                                                                     krbn::core_configuration::error_handling::loose);
    configuration_monitor_->core_configuration_updated.connect([](auto&& weak_core_configuration) {
      if (auto c = weak_core_configuration.lock()) {
        //
        // menu
        //

        if (c->get_global_configuration().get_show_in_menu_bar() ||
            c->get_global_configuration().get_show_profile_name_in_menu_bar()) {
          services_utility::register_menu_agent();
        } else {
          services_utility::unregister_menu_agent();
        }

        //
        // multitouch_extension
        //

        if (c->get_machine_specific().get_entry().get_enable_multitouch_extension()) {
          services_utility::register_multitouch_extension_agent();
        } else {
          services_utility::unregister_multitouch_extension_agent();
        }

        //
        // notification_window
        //

        if (c->get_global_configuration().get_enable_notification_window()) {
          services_utility::register_notification_window_agent();
        } else {
          services_utility::unregister_notification_window_agent();
        }

        //
        // settings alert
        //

        if (c->get_selected_profile().get_virtual_hid_keyboard()->get_keyboard_type_v2() == "") {
          application_launcher::launch_settings();
        }
      }
    });

    // updater_process_manager_

    updater_process_manager_ = std::make_unique<updater_process_manager>(configuration_monitor_);

    // system_preferences_monitor_

    system_preferences_monitor_ = std::make_unique<pqrs::osx::system_preferences_monitor>(weak_dispatcher_);

    system_preferences_monitor_->system_preferences_changed.connect([this](auto&& properties_ptr) {
      if (grabber_client_) {
        grabber_client_->async_system_preferences_updated(properties_ptr);
      }
    });

    system_preferences_monitor_->async_start(std::chrono::milliseconds(3000));

    // frontmost_application_monitor
    //
    // `frontmost_application_monitor` does not work properly in karabiner_grabber after fast user switching.
    // Therefore, we have to use `frontmost_application_monitor` in `console_user_server`.

    pqrs::osx::frontmost_application_monitor::monitor::initialize_shared_monitor(weak_dispatcher_);

    if (auto m = pqrs::osx::frontmost_application_monitor::monitor::get_shared_monitor().lock()) {
      m->frontmost_application_changed.connect([this](auto&& application_ptr) {
        if (application_ptr) {
          if (application_ptr->get_bundle_identifier() == "org.pqrs.Karabiner.EventViewer" ||
              application_ptr->get_bundle_identifier() == "org.pqrs.Karabiner-EventViewer") {
            return;
          }

          if (grabber_client_) {
            grabber_client_->async_frontmost_application_changed(application_ptr);
          }
        }
      });

      m->trigger();
    }

    // input_source_monitor_

    input_source_monitor_ = std::make_unique<pqrs::osx::input_source_monitor>(
        pqrs::dispatcher::extra::get_shared_dispatcher());

    input_source_monitor_->input_source_changed.connect([this](auto&& input_source_ptr) {
      if (input_source_ptr && grabber_client_) {
        auto properties = std::make_shared<pqrs::osx::input_source::properties>(*input_source_ptr);
        grabber_client_->async_input_source_changed(properties);
      }
    });

    input_source_monitor_->async_start();

    // input_source_selector_

    input_source_selector_ = std::make_unique<pqrs::osx::input_source_selector::selector>(weak_dispatcher_);

    // software_function_handler_

    software_function_handler_ = std::make_unique<software_function_handler>();

    // shell_command_handler_

    shell_command_handler_ = std::make_unique<shell_command_handler>();

    // Start configuration_monitor_

    configuration_monitor_->async_start();
  }

  void stop_child_components(void) {
    updater_process_manager_ = nullptr;
    system_preferences_monitor_ = nullptr;
    pqrs::osx::frontmost_application_monitor::monitor::terminate_shared_monitor();
    input_source_monitor_ = nullptr;
    input_source_selector_ = nullptr;
    software_function_handler_ = nullptr;
    shell_command_handler_ = nullptr;

    configuration_monitor_ = nullptr;
  }

  // Core components

  std::unique_ptr<version_monitor> version_monitor_;

  std::optional<bool> on_console_;
  std::unique_ptr<pqrs::osx::session::monitor> session_monitor_;
  std::shared_ptr<grabber_client> grabber_client_;

  // Child components

  std::shared_ptr<configuration_monitor> configuration_monitor_;
  std::unique_ptr<updater_process_manager> updater_process_manager_;
  std::unique_ptr<pqrs::osx::system_preferences_monitor> system_preferences_monitor_;
  std::unique_ptr<pqrs::osx::input_source_monitor> input_source_monitor_;
  std::unique_ptr<pqrs::osx::input_source_selector::selector> input_source_selector_;
  std::unique_ptr<software_function_handler> software_function_handler_;
  std::unique_ptr<shell_command_handler> shell_command_handler_;
};
} // namespace console_user_server
} // namespace krbn
