#pragma once

// `krbn::console_user_server::components_manager` can be used safely in a multi-threaded environment.

#include "application_launcher.hpp"
#include "chrono_utility.hpp"
#include "components_manager_killer.hpp"
#include "constants.hpp"
#include "core_service_client.hpp"
#include "filesystem_utility.hpp"
#include "logger.hpp"
#include "monitor/version_monitor.hpp"
#include "receiver.hpp"
#include "services_utility.hpp"
#include "shell_command_handler.hpp"
#include "software_function_handler.hpp"
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

      stop_core_service_client();
      start_core_service_client();
    });
  }

  virtual ~components_manager(void) {
    detach_from_dispatcher([this] {
      stop_core_service_client();

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
  void start_core_service_client(void) {
    if (core_service_client_) {
      return;
    }

    if (on_console_ != true) {
      return;
    }

    // Note:
    // The socket file path length must be <= 103 because sizeof(sockaddr_un.sun_path) == 104.
    // So we use the shorten name console_user_server_core_service_client -> con_usr_srv_cs_clnt.
    //
    // Example:
    // `/Library/Application Support/org.pqrs/tmp/user/501/con_usr_srv_cs_clnt/17d502ed0dd6f828.sock`

    core_service_client_ = std::make_shared<core_service_client>("con_usr_srv_cs_clnt");

    core_service_client_->connected.connect([this] {
      version_monitor_->async_manual_check();

      if (core_service_client_) {
        core_service_client_->async_connect_console_user_server();
      }

      stop_child_components();
      start_child_components();
    });

    core_service_client_->connect_failed.connect([this](auto&& error_code) {
      version_monitor_->async_manual_check();

      stop_child_components();
    });

    core_service_client_->closed.connect([this] {
      version_monitor_->async_manual_check();

      stop_child_components();
    });

    core_service_client_->received.connect([this](auto&& buffer, auto&& sender_endpoint) {
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

    core_service_client_->async_start();
  }

  void stop_core_service_client(void) {
    core_service_client_ = nullptr;
    stop_child_components();
  }

  void start_child_components(void) {
    // system_preferences_monitor_

    system_preferences_monitor_ = std::make_unique<pqrs::osx::system_preferences_monitor>(weak_dispatcher_);

    system_preferences_monitor_->system_preferences_changed.connect([this](auto&& properties_ptr) {
      if (core_service_client_) {
        core_service_client_->async_system_preferences_updated(properties_ptr);
      }
    });

    system_preferences_monitor_->async_start(std::chrono::milliseconds(3000));

    // frontmost_application_monitor
    //
    // `frontmost_application_monitor` does not work properly in Karabiner-Core-Service after fast user switching.
    // Therefore, we have to use `frontmost_application_monitor` in `console_user_server`.

    pqrs::osx::frontmost_application_monitor::monitor::initialize_shared_monitor(weak_dispatcher_);

    if (auto m = pqrs::osx::frontmost_application_monitor::monitor::get_shared_monitor().lock()) {
      m->frontmost_application_changed.connect([this](auto&& application_ptr) {
        if (application_ptr) {
          if (software_function_handler_) {
            software_function_handler_->add_frontmost_application_history(*application_ptr);
          }

          //
          // Notify the core_service of frontmost application changed.
          //

          if (application_ptr->get_bundle_identifier() == "org.pqrs.Karabiner.EventViewer" ||
              application_ptr->get_bundle_identifier() == "org.pqrs.Karabiner-EventViewer") {
            return;
          }

          if (core_service_client_) {
            core_service_client_->async_frontmost_application_changed(application_ptr);
          }
        }
      });

      m->trigger();
    }

    // input_source_monitor_

    input_source_monitor_ = std::make_unique<pqrs::osx::input_source_monitor>(
        pqrs::dispatcher::extra::get_shared_dispatcher());

    input_source_monitor_->input_source_changed.connect([this](auto&& input_source_ptr) {
      if (input_source_ptr && core_service_client_) {
        auto properties = std::make_shared<pqrs::osx::input_source::properties>(*input_source_ptr);
        core_service_client_->async_input_source_changed(properties);
      }
    });

    input_source_monitor_->async_start();

    // input_source_selector_

    input_source_selector_ = std::make_shared<pqrs::osx::input_source_selector::selector>(weak_dispatcher_);

    // shell_command_handler_

    shell_command_handler_ = std::make_shared<shell_command_handler>();

    // software_function_handler_

    software_function_handler_ = std::make_shared<software_function_handler>();

    // receiver_

    receiver_ = std::make_unique<receiver>(input_source_selector_,
                                           shell_command_handler_,
                                           software_function_handler_);
  }

  void stop_child_components(void) {
    receiver_ = nullptr;

    system_preferences_monitor_ = nullptr;
    pqrs::osx::frontmost_application_monitor::monitor::terminate_shared_monitor();
    input_source_monitor_ = nullptr;
    input_source_selector_ = nullptr;
    shell_command_handler_ = nullptr;
    software_function_handler_ = nullptr;
  }

  // Core components

  std::unique_ptr<version_monitor> version_monitor_;

  std::optional<bool> on_console_;
  std::unique_ptr<pqrs::osx::session::monitor> session_monitor_;
  std::shared_ptr<core_service_client> core_service_client_;

  // Child components

  std::unique_ptr<pqrs::osx::system_preferences_monitor> system_preferences_monitor_;
  std::unique_ptr<pqrs::osx::input_source_monitor> input_source_monitor_;

  std::shared_ptr<pqrs::osx::input_source_selector::selector> input_source_selector_;
  std::shared_ptr<shell_command_handler> shell_command_handler_;
  std::shared_ptr<software_function_handler> software_function_handler_;

  std::unique_ptr<receiver> receiver_;
};
} // namespace console_user_server
} // namespace krbn
