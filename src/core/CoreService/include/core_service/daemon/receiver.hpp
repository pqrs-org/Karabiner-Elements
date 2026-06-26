#pragma once

// `krbn::core_service::daemon::receiver` can be used safely in a multi-threaded environment.

#include "app_icon.hpp"
#include "application_launcher.hpp"
#include "components_manager_killer.hpp"
#include "console_user_server_client.hpp"
#include "constants.hpp"
#include "core_service/daemon/core_service_daemon_state_manager.hpp"
#include "device_grabber.hpp"
#include "filesystem_utility.hpp"
#include "types.hpp"
#include "types/core_service_daemon_state.hpp"
#include <array>
#include <pqrs/dispatcher.hpp>
#include <pqrs/filesystem.hpp>
#include <pqrs/karabiner/driverkit/virtual_hid_device_service/utility.hpp>
#include <pqrs/osx/session.hpp>
#include <pqrs/osx/system_preferences.hpp>
#include <pqrs/osx/system_preferences/extra/nlohmann_json.hpp>
#include <pqrs/unix_domain_stream.hpp>
#include <unordered_set>
#include <vector>

namespace krbn::core_service::daemon {
class receiver final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  receiver(const receiver&) = delete;

  receiver(std::optional<uid_t> current_console_user_id,
           std::weak_ptr<core_service_daemon_state_manager> weak_core_service_daemon_state_manager)
      : dispatcher_client(),
        current_console_user_id_(current_console_user_id),
        weak_core_service_daemon_state_manager_(weak_core_service_daemon_state_manager) {
    prepare_karabiner_core_service_socket_directory();

    if (auto m = weak_core_service_daemon_state_manager_.lock()) {
      core_service_daemon_state_manager_connection_ = m->core_service_daemon_state_changed.connect([this](const auto& core_service_daemon_state) {
        enqueue_to_dispatcher([this, core_service_daemon_state] {
          send_core_service_daemon_state(core_service_daemon_state);
        });
      });
    }

    //
    // Setup server_
    //

    auto options = pqrs::unix_domain_stream::server_options(
        {
            .max_message_size = constants::unix_domain_stream_max_message_size,
        },
        {
            .bind_retry_interval = std::chrono::milliseconds(1000),
            .socket_path_health_check_interval = std::chrono::milliseconds(3000),
        });

    auto socket_file_path = karabiner_core_service_socket_file_path();

    server_ = std::make_unique<pqrs::unix_domain_stream::server>(
        weak_dispatcher_,
        socket_file_path,
        options,
        [](const auto& peer_credentials) {
          auto result = get_shared_codesign_manager()->same_team_id(peer_credentials.pid);
          if (!result) {
            // During an update, retrieving the Team ID may fail, causing an error once.
            // Since this can occur during normal use, treat it as debug rather than warn.
            logger::get_logger()->debug("receiver: peer is not code-signed with same Team ID");
          }
          return result;
        });

    server_->bound.connect([this, socket_file_path] {
      logger::get_logger()->info("receiver: bound");

      auto chown_uid = current_console_user_id_.value_or(uid_t(0));
      logger::get_logger()->info("receiver: chown socket: {0}", chown_uid);
      chown(socket_file_path.c_str(), chown_uid, 0);

      chmod(socket_file_path.c_str(), 0600);
    });

    server_->bind_failed.connect([this](auto&& error_code) {
      logger::get_logger()->error("receiver: bind_failed");

      // If the socket directory is deleted for any reason,
      // bind_failed will be triggered, so recreate the directory each time.
      prepare_karabiner_core_service_socket_directory();
    });

    server_->closed.connect([] {
      logger::get_logger()->info("receiver: closed");
    });

    server_->peer_connected.connect([](auto, auto&&) {
      // Do nothing
    });

    server_->peer_closed.connect([this](auto peer_id) {
      handle_peer_closed(peer_id);
    });

    server_->peer_error_occurred.connect([](auto peer_id, auto&& error_code) {
      logger::get_logger()->debug("receiver: peer_error_occurred ({0}): {1}", peer_id, error_code.message());
    });

    server_->received.connect([](auto, auto&&) {
      // Do nothing
    });

    server_->request_received.connect([this](auto peer_id, auto request_id, auto&& buffer) {
      handle_request(peer_id,
                     request_id,
                     buffer);
    });

    server_->async_start();

    //
    // Setup console_user_server_client_
    //

    if (current_console_user_id_) {
      console_user_server_client_ = std::make_unique<console_user_server_client>(*current_console_user_id_);

      console_user_server_client_->connected.connect([this] {
        console_user_server_client_->async_get_user_core_configuration_file_path();

        if (auto m = weak_core_service_daemon_state_manager_.lock()) {
          send_core_service_daemon_state(m->copy_state());
        }

        request_core_service_bundle_permission_check_result_refresh();
      });

      // If the console_user_server isn't running (i.e., operating under system_core_configuration),
      // connect_failed will keep being called, so we won't perform any operations on the device_grabber.
      console_user_server_client_->connect_failed.connect([](auto&& error_code) {
        // Do nothing
      });

      console_user_server_client_->closed.connect([this] {
        stop_device_grabber();
        start_grabbing_if_system_core_configuration_file_exists();
      });

      console_user_server_client_->received.connect([this](auto&& ot,
                                                           auto&& json) {
        try {
          switch (ot) {
            case operation_type::user_core_configuration_file_path:
              stop_device_grabber();
              start_device_grabber(json["user_core_configuration_file_path"].template get<std::string>());
              break;

            default:
              break;
          }
        } catch (std::exception& e) {
          logger::get_logger()->error("received data is corrupted");
        }
      });

      console_user_server_client_->async_start();
    }

    //
    // Start device_grabber
    //

    start_grabbing_if_system_core_configuration_file_exists();
    enqueue_update_driver_activated();

    logger::get_logger()->info("receiver is initialized");
  }

  ~receiver() override {
    detach_from_dispatcher([this] {
      core_service_daemon_state_manager_connection_.disconnect();
      server_ = nullptr;
      console_user_server_client_ = nullptr;
      stop_device_grabber();
    });

    logger::get_logger()->info("receiver is terminated");
  }

private:
  void async_send(pqrs::unix_domain_stream::peer_id peer_id,
                  nlohmann::json json) {
    if (server_) {
      server_->async_send(peer_id,
                          nlohmann::json::to_msgpack(json));
    }
  }

  void async_respond(pqrs::unix_domain_stream::peer_id peer_id,
                     pqrs::unix_domain_stream::request_id request_id,
                     nlohmann::json json) {
    if (server_) {
      server_->async_respond(peer_id,
                             request_id,
                             nlohmann::json::to_msgpack(json));
    }
  }

  void async_respond_none(pqrs::unix_domain_stream::peer_id peer_id,
                          pqrs::unix_domain_stream::request_id request_id) {
    async_respond(peer_id,
                  request_id,
                  nlohmann::json{
                      {"operation_type", operation_type::none},
                  });
  }

  void handle_peer_closed(pqrs::unix_domain_stream::peer_id peer_id) {
    temporarily_ignore_all_devices_peer_ids_.erase(peer_id);

    // Restore the flags when all clients have closed.
    if (temporarily_ignore_all_devices_peer_ids_.empty()) {
      if (device_grabber_) {
        device_grabber_->async_set_temporarily_ignore_all_devices(false);
      }
    }

    if (multitouch_extension_peer_id_ == peer_id) {
      logger::get_logger()->info("multitouch_extension is closed.");

      multitouch_extension_peer_id_ = std::nullopt;
      clear_multitouch_extension_environment_variables();
    }

    if (core_service_agent_peer_id_ == peer_id) {
      core_service_agent_peer_id_ = std::nullopt;
    }
  }

  void handle_request(pqrs::unix_domain_stream::peer_id peer_id,
                      pqrs::unix_domain_stream::request_id request_id,
                      pqrs::not_null_shared_ptr_t<std::vector<uint8_t>> buffer) {
    if (buffer->empty()) {
      server_->async_close_peer(peer_id);
      return;
    }

    try {
      nlohmann::json json = nlohmann::json::from_msgpack(*buffer);
      switch (json.at("operation_type").get<operation_type>()) {
        case operation_type::core_service_bundle_permission_check_result:
          if (auto m = weak_core_service_daemon_state_manager_.lock()) {
            core_service_agent_peer_id_ = peer_id;

            // If the required permissions were missing when this process started,
            // and a newly launched process confirms that the permissions are now granted,
            // restart this process.
            // (The actual restart is handled by launchd, so this process just exits.)

            auto iohid_listen_event_allowed = json.at("iohid_listen_event_allowed").get<bool>();
            auto accessibility_process_trusted = json.at("accessibility_process_trusted").get<bool>();
            auto restart_required =
                !required_permissions_granted() &&
                iohid_listen_event_allowed &&
                accessibility_process_trusted;

            auto result = core_service_permission_check_result();
            result.set_iohid_listen_event_allowed(iohid_listen_event_allowed);
            result.set_accessibility_process_trusted(accessibility_process_trusted);

            m->set_bundle_permission_check_result(result);

            if (restart_required) {
              logger::get_logger()->info("The required permissions are granted. Restarting core daemons.");

              if (auto killer = components_manager_killer::get_shared_components_manager_killer()) {
                killer->async_kill();
              }
            }
          }
          async_respond_none(peer_id,
                             request_id);
          break;

        case operation_type::system_preferences_updated:
          system_preferences_properties_ = json.at("system_preferences_properties")
                                               .get<pqrs::osx::system_preferences::properties>();
          if (device_grabber_) {
            device_grabber_->async_set_system_preferences_properties(system_preferences_properties_);
          }
          logger::get_logger()->info("`system_preferences` is updated.");
          async_respond_none(peer_id,
                             request_id);
          break;

        case operation_type::frontmost_application_changed: {
          auto app = json.at("frontmost_application").get<application>();

          //
          // Synchronize frontmost_application_changed to console_user_server
          //

          if (console_user_server_client_) {
            console_user_server_client_->async_frontmost_application_changed(app);
          }

          //
          // Update environment_variables in device_grabber
          //

          if (app.get_bundle_identifier() != "org.pqrs.Karabiner-EventViewer") {
            frontmost_application_ = app;
            if (device_grabber_) {
              device_grabber_->async_post_frontmost_application_changed_event(app);
            }
          }
          async_respond_none(peer_id,
                             request_id);
          break;
        }

        case operation_type::focused_ui_element_changed: {
          auto element = json.at("focused_ui_element").get<focused_ui_element>();

          //
          // Synchronize focused_ui_element_changed to console_user_server
          //

          if (console_user_server_client_) {
            console_user_server_client_->async_focused_ui_element_changed(element);
          }

          //
          // Update accessibility.* variables
          //

          focused_ui_element_ = element;
          set_focused_ui_element_variables();
          async_respond_none(peer_id,
                             request_id);
          break;
        }

        case operation_type::input_source_changed:
          input_source_properties_ = json.at("input_source_properties")
                                         .get<pqrs::osx::input_source::properties>();
          if (device_grabber_) {
            device_grabber_->async_post_input_source_changed_event(input_source_properties_);
          }
          async_respond_none(peer_id,
                             request_id);
          break;

        case operation_type::temporarily_ignore_all_devices: {
          auto value = json.at("value").get<bool>();

          if (device_grabber_) {
            device_grabber_->async_set_temporarily_ignore_all_devices(value);
          }

          temporarily_ignore_all_devices_peer_ids_.insert(peer_id);
          async_respond_none(peer_id,
                             request_id);
          break;
        }

        case operation_type::get_manipulator_environment:
          if (device_grabber_) {
            device_grabber_->async_invoke_with_manipulator_environment(
                [this, peer_id, request_id](auto&& manipulator_environment) {
                  async_respond(peer_id,
                                request_id,
                                nlohmann::json{
                                    {"operation_type", operation_type::manipulator_environment},
                                    {"manipulator_environment", manipulator_environment.to_json()},
                                });
                });
          } else {
            async_respond_none(peer_id,
                               request_id);
          }
          break;

        case operation_type::connect_multitouch_extension:
          logger::get_logger()->info("multitouch_extension is connected.");
          multitouch_extension_peer_id_ = peer_id;
          async_respond_none(peer_id,
                             request_id);
          break;

        case operation_type::set_app_icon: {
          // `set_app_icon` requires root privileges.
          auto number = json.at("number").get<int>();

          logger::get_logger()->info("set_app_icon {0}", number);

          app_icon(number).async_save_to_file(constants::get_system_app_icon_configuration_file_path());

          application_launcher::launch_app_icon_switcher();

          async_respond_none(peer_id,
                             request_id);
          break;
        }

        case operation_type::set_variables:
          if (device_grabber_) {
            for (const auto& [k, v] : json.at("variables").items()) {
              device_grabber_->async_post_set_variable_event(
                  manipulator_environment_variable_set_variable(
                      k,
                      v.get<manipulator_environment_variable_value>(),
                      nullptr,
                      std::nullopt,
                      nullptr));
            }
          }
          async_respond_none(peer_id,
                             request_id);
          break;

        case operation_type::clear_user_variables:
          if (device_grabber_) {
            device_grabber_->async_invoke_with_manipulator_environment(
                [this, peer_id, request_id](auto&& manipulator_environment) {
                  for (const auto& name : manipulator_environment.get_variable_names()) {
                    if (name.starts_with("system.") ||
                        name.starts_with("accessibility.")) {
                      continue;
                    }

                    device_grabber_->async_post_set_variable_event(
                        manipulator_environment_variable_set_variable(
                            name,
                            std::nullopt,
                            nullptr,
                            std::nullopt,
                            nullptr,
                            manipulator_environment_variable_set_variable::type::unset));
                  }

                  async_respond_none(peer_id,
                                     request_id);
                });
          } else {
            async_respond_none(peer_id,
                               request_id);
          }
          break;

        case operation_type::get_connected_devices:
          if (device_grabber_) {
            device_grabber_->async_invoke_with_connected_devices(
                [this, peer_id, request_id](auto&& connected_devices_json) {
                  async_respond(peer_id,
                                request_id,
                                nlohmann::json{
                                    {"operation_type", operation_type::connected_devices},
                                    {"connected_devices", connected_devices_json},
                                });
                });
          } else {
            async_respond_none(peer_id,
                               request_id);
          }
          break;

        case operation_type::get_notification_message:
          if (device_grabber_) {
            device_grabber_->async_invoke_with_notification_message(
                [this, peer_id, request_id](auto&& message) {
                  async_respond(peer_id,
                                request_id,
                                nlohmann::json{
                                    {"operation_type", operation_type::notification_message},
                                    {"notification_message", message},
                                });
                });
          } else {
            async_respond_none(peer_id,
                               request_id);
          }
          break;

        case operation_type::get_system_variables:
          if (device_grabber_) {
            device_grabber_->async_invoke_with_manipulator_environment(
                [this, peer_id, request_id](auto&& manipulator_environment) {
                  async_respond(
                      peer_id,
                      request_id,
                      nlohmann::json{
                          {"operation_type", operation_type::system_variables},
                          {
                              "system_variables",
                              {
                                  {
                                      "system.now.milliseconds",
                                      manipulator_environment.get_variable("system.now.milliseconds"),
                                  },
                                  {
                                      "system.scroll_direction_is_natural",
                                      manipulator_environment.get_variable("system.scroll_direction_is_natural"),
                                  },
                                  {
                                      "system.temporarily_ignore_all_devices",
                                      manipulator_environment.get_variable("system.temporarily_ignore_all_devices"),
                                  },
                                  {
                                      "system.use_fkeys_as_standard_function_keys",
                                      manipulator_environment.get_variable("system.use_fkeys_as_standard_function_keys"),
                                  },
                              },

                              // Note:
                              // The accessibility.* variables contain information such as window titles,
                              // so they should not be exposed to other applications via the CLI
                              // and are therefore excluded from the return value.
                          }});
                });
          } else {
            async_respond_none(peer_id,
                               request_id);
          }
          break;

        case operation_type::get_multitouch_extension_variables:
          if (device_grabber_) {
            device_grabber_->async_invoke_with_manipulator_environment(
                [this, peer_id, request_id](auto&& manipulator_environment) {
                  std::unordered_map<std::string, manipulator_environment_variable_value> variables;
                  for (const auto& name : multitouch_extension_environment_variable_names) {
                    variables[name] = manipulator_environment.get_variable(name);
                  }

                  async_respond(peer_id,
                                request_id,
                                nlohmann::json{
                                    {"operation_type", operation_type::multitouch_extension_variables},
                                    {"multitouch_extension_variables", variables}});
                });
          } else {
            async_respond_none(peer_id,
                               request_id);
          }
          break;

        default:
          server_->async_close_peer(peer_id);
          break;
      }
    } catch (std::exception& e) {
      logger::get_logger()->error("received data is corrupted");
      server_->async_close_peer(peer_id);
    }
  }

  void enqueue_update_driver_activated() {
    enqueue_to_dispatcher(
        [this] {
          if (auto m = weak_core_service_daemon_state_manager_.lock()) {
            m->set_driver_activated(
                pqrs::karabiner::driverkit::virtual_hid_device_service::utility::driver_running());
          }

          enqueue_update_driver_activated();
        },
        when_now() + std::chrono::seconds(1));
  }

  std::filesystem::path karabiner_core_service_socket_file_path() const {
    return constants::get_karabiner_core_service_socket_file_path();
  }

  void prepare_karabiner_core_service_socket_directory() const {
    filesystem_utility::create_base_directories(current_console_user_id_);
  }

  void start_grabbing_if_system_core_configuration_file_exists() {
    if (!required_permissions_granted()) {
      logger::get_logger()->info("device_grabber is not started because the required permissions are not granted.");
      return;
    }

    auto file_path = constants::get_system_core_configuration_file_path();
    if (pqrs::filesystem::exists(file_path)) {
      stop_device_grabber();
      start_device_grabber(file_path);
    }
  }

  void start_device_grabber(const std::string& configuration_file_path) {
    if (device_grabber_) {
      return;
    }

    if (!required_permissions_granted()) {
      logger::get_logger()->info("device_grabber is not started because the required permissions are not granted.");
      return;
    }

    clear_device_grabber_state();

    device_grabber_ = std::make_unique<device_grabber>(console_user_server_client_,
                                                       weak_core_service_daemon_state_manager_);

    device_grabber_->async_set_system_preferences_properties(system_preferences_properties_);
    device_grabber_->async_post_frontmost_application_changed_event(frontmost_application_);
    set_focused_ui_element_variables();
    device_grabber_->async_post_input_source_changed_event(input_source_properties_);

    device_grabber_->async_start(configuration_file_path,
                                 current_console_user_id_);

    logger::get_logger()->info("device_grabber is started.");
  }

  void stop_device_grabber() {
    if (!device_grabber_) {
      return;
    }

    device_grabber_ = nullptr;
    clear_device_grabber_state();

    logger::get_logger()->info("device_grabber is stopped.");
  }

  bool required_permissions_granted() const {
    if (auto m = weak_core_service_daemon_state_manager_.lock()) {
      if (auto result = m->copy_current_process_permission_check_result()) {
        return result->required_permissions_granted();
      }
    }

    return false;
  }

  void clear_device_grabber_state() {
    if (auto m = weak_core_service_daemon_state_manager_.lock()) {
      m->set_virtual_hid_device_service_client_connected(std::nullopt);
      m->set_driver_activated(
          pqrs::karabiner::driverkit::virtual_hid_device_service::utility::driver_running());
      m->set_driver_connected(std::nullopt);
      m->set_driver_version_mismatched(std::nullopt);
      m->set_virtual_hid_keyboard_ready(std::nullopt);
    }
  }

  void set_focused_ui_element_variables() {
    if (device_grabber_) {
      device_grabber_->async_post_set_variable_event(
          manipulator_environment_variable_set_variable(
              "accessibility.focused_ui_element.role_string",
              manipulator_environment_variable_value(focused_ui_element_.get_role().value_or("")),
              nullptr,
              std::nullopt,
              nullptr));

      device_grabber_->async_post_set_variable_event(
          manipulator_environment_variable_set_variable(
              "accessibility.focused_ui_element.subrole_string",
              manipulator_environment_variable_value(focused_ui_element_.get_subrole().value_or("")),
              nullptr,
              std::nullopt,
              nullptr));

      device_grabber_->async_post_set_variable_event(
          manipulator_environment_variable_set_variable(
              "accessibility.focused_ui_element.title_string",
              manipulator_environment_variable_value(focused_ui_element_.get_title().value_or("")),
              nullptr,
              std::nullopt,
              nullptr));

      device_grabber_->async_post_set_variable_event(
          manipulator_environment_variable_set_variable(
              "accessibility.focused_ui_element.window_position_x",
              manipulator_environment_variable_value(
                  static_cast<int64_t>(focused_ui_element_.get_window_position_x().value_or(0))),
              nullptr,
              std::nullopt,
              nullptr));

      device_grabber_->async_post_set_variable_event(
          manipulator_environment_variable_set_variable(
              "accessibility.focused_ui_element.window_position_y",
              manipulator_environment_variable_value(
                  static_cast<int64_t>(focused_ui_element_.get_window_position_y().value_or(0))),
              nullptr,
              std::nullopt,
              nullptr));

      device_grabber_->async_post_set_variable_event(
          manipulator_environment_variable_set_variable(
              "accessibility.focused_ui_element.window_size_width",
              manipulator_environment_variable_value(
                  static_cast<int64_t>(focused_ui_element_.get_window_size_width().value_or(0))),
              nullptr,
              std::nullopt,
              nullptr));

      device_grabber_->async_post_set_variable_event(
          manipulator_environment_variable_set_variable(
              "accessibility.focused_ui_element.window_size_height",
              manipulator_environment_variable_value(
                  static_cast<int64_t>(focused_ui_element_.get_window_size_height().value_or(0))),
              nullptr,
              std::nullopt,
              nullptr));
    }
  }

  void send_core_service_daemon_state(const core_service_daemon_state& core_service_daemon_state) {
    if (console_user_server_client_) {
      console_user_server_client_->async_core_service_daemon_state(core_service_daemon_state);
    }
  }

  void request_core_service_bundle_permission_check_result_refresh() {
    if (!core_service_agent_peer_id_) {
      return;
    }

    async_send(
        *core_service_agent_peer_id_,
        nlohmann::json{
            {"operation_type", operation_type::refresh_core_service_bundle_permission_check_result},
        });
  }

  void clear_multitouch_extension_environment_variables() {
    if (device_grabber_) {
      for (const auto& name : multitouch_extension_environment_variable_names) {
        device_grabber_->async_post_set_variable_event(
            manipulator_environment_variable_set_variable(
                name,
                std::nullopt,
                nullptr,
                std::nullopt,
                nullptr,
                manipulator_environment_variable_set_variable::type::unset));
      }
    }
  }

  static constexpr std::array multitouch_extension_environment_variable_names{
      "multitouch_extension_finger_count_upper_quarter_area",
      "multitouch_extension_finger_count_lower_quarter_area",
      "multitouch_extension_finger_count_left_quarter_area",
      "multitouch_extension_finger_count_right_quarter_area",
      "multitouch_extension_finger_count_upper_half_area",
      "multitouch_extension_finger_count_lower_half_area",
      "multitouch_extension_finger_count_left_half_area",
      "multitouch_extension_finger_count_right_half_area",
      "multitouch_extension_finger_count_total",
      "multitouch_extension_palm_count_upper_half_area",
      "multitouch_extension_palm_count_lower_half_area",
      "multitouch_extension_palm_count_left_half_area",
      "multitouch_extension_palm_count_right_half_area",
      "multitouch_extension_palm_count_total",
  };

  std::optional<uid_t> current_console_user_id_;
  std::weak_ptr<core_service_daemon_state_manager> weak_core_service_daemon_state_manager_;
  nod::scoped_connection core_service_daemon_state_manager_connection_;

  std::unique_ptr<pqrs::unix_domain_stream::server> server_;
  std::shared_ptr<console_user_server_client> console_user_server_client_;
  std::unique_ptr<device_grabber> device_grabber_;
  std::optional<pqrs::unix_domain_stream::peer_id> core_service_agent_peer_id_;
  std::optional<pqrs::unix_domain_stream::peer_id> multitouch_extension_peer_id_;
  std::unordered_set<pqrs::unix_domain_stream::peer_id> temporarily_ignore_all_devices_peer_ids_;

  pqrs::osx::system_preferences::properties system_preferences_properties_;
  application frontmost_application_;
  focused_ui_element focused_ui_element_;
  pqrs::osx::input_source::properties input_source_properties_;
};
} // namespace krbn::core_service::daemon
