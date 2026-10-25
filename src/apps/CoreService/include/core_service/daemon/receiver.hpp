#pragma once

// `krbn::core_service::daemon::receiver` can be used safely in a multi-threaded environment.

#include "app_icon.hpp"
#include "application_launcher.hpp"
#include "console_user_server_peer.hpp"
#include "constants.hpp"
#include "core_service/daemon/core_service_daemon_state_manager.hpp"
#include "device_grabber.hpp"
#include "filesystem_utility.hpp"
#include "process_lifecycle_manager.hpp"
#include "types.hpp"
#include "types/core_service_daemon_state.hpp"
#include <array>
#include <filesystem>
#include <functional>
#include <memory>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/session.hpp>
#include <pqrs/osx/system_preferences.hpp>
#include <pqrs/osx/system_preferences/extra/nlohmann_json.hpp>
#include <pqrs/osx/xpc.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace krbn::core_service::daemon {
namespace xpc = pqrs::osx::xpc;

class receiver final : public pqrs::dispatcher::extra::dispatcher_client {
  pqrs::dispatcher::extra::dispatcher_client_constructor_exception_guard dispatcher_client_constructor_guard_{*this};

public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(std::optional<uid_t>)> console_user_id_changed;
  nod::signal<void(const xpc::listener_recovery_request&)> listener_recovery_required;

  // Methods

  receiver(const receiver&) = delete;

  receiver(std::optional<uid_t> current_console_user_id,
           std::weak_ptr<core_service_daemon_state_manager> weak_core_service_daemon_state_manager)
      : current_console_user_id_(current_console_user_id),
        weak_core_service_daemon_state_manager_(weak_core_service_daemon_state_manager) {
    dispatcher_client_constructor_guard_.initialize(
        [&] {
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

          server_ = std::make_shared<xpc::listener>(
              weak_dispatcher_,
              xpc::listener_options({
                  .common_parameters = {
                      .signing_requirement = get_shared_codesign_manager()->xpc_signing_requirement({
                          "org.pqrs.Karabiner-CLI",
                          "org.pqrs.Karabiner-Console-User-Server",
                          "org.pqrs.Karabiner-Core-Service",
                          "org.pqrs.Karabiner-Elements.Settings",
                          "org.pqrs.Karabiner-EventViewer",
                          "org.pqrs.Karabiner-MultitouchExtension",
                      }),
                      .max_message_size = constants::xpc_max_message_size,
                  },
                  .listener_parameters = {
                      .service_name = constants::get_core_service_daemon_mach_service_name(),
                  },
              }));

          server_->listener_started.connect([] {
            logger::get_logger()->debug("receiver: listener_started");
          });

          server_->listener_recovery_required.connect([this](const auto& request) {
            listener_recovery_required(request);
          });

          server_->peer_ready.connect([](const auto&) {
            // Do nothing
          });

          server_->peer_interrupted.connect([this](const auto& context) {
            handle_peer_closed(context.id);
          });

          server_->peer_invalidated.connect([this](const auto& context,
                                                   const auto& error_code) {
            logger::get_logger()->debug("receiver: peer_invalidated ({0}): {1}",
                                        context.id,
                                        error_code.message());

            handle_peer_closed(context.id);
          });

          server_->message_received.connect([](const auto&, auto&&) {
            // Do nothing
          });

          server_->request_received.connect([this](const auto& context,
                                                   auto&& buffer) {
            handle_request(context,
                           buffer);
          });

          server_->async_start();

          //
          // Start device_grabber
          //

          start_grabbing_if_system_core_configuration_file_exists();

          logger::get_logger()->debug("receiver is initialized");
        },
        [this] {
          cleanup();
        });
  }

  ~receiver() override {
    detach_from_dispatcher([this] {
      cleanup();
    });

    logger::get_logger()->debug("receiver is terminated");
  }

private:
  void cleanup() {
    core_service_daemon_state_manager_connection_.disconnect();
    console_user_server_peer_id_ = std::nullopt;
    console_user_server_peer_ = nullptr;
    stop_device_grabber();
    server_ = nullptr;
  }

  void async_request(xpc::peer_id peer_id,
                     nlohmann::json json) {
    if (server_) {
      server_->async_request(
          xpc::listener::async_request_parameters{
              .id = peer_id,
              .data = std::make_shared<const std::vector<uint8_t>>(nlohmann::json::to_msgpack(json)),
              .completion = [](auto result) {
                if (!result) {
                  logger::get_logger()->debug("receiver: request failed: {0}", result.error().message());
                }
              },
          });
    }
  }

  void async_respond(xpc::reply_token reply,
                     nlohmann::json json) {
    if (server_) {
      server_->async_reply(reply,
                           std::make_shared<const std::vector<uint8_t>>(nlohmann::json::to_msgpack(json)));
    }
  }

  void async_respond_none(xpc::reply_token reply) {
    async_respond(reply,
                  nlohmann::json{
                      {"operation_type", operation_type::none},
                  });
  }

  void handle_peer_closed(xpc::peer_id peer_id) {
    if (temporarily_ignored_device_ids_by_peer_.erase(peer_id) > 0) {
      update_temporarily_ignored_device_ids();
    }
    connected_devices_observer_peer_ids_.erase(peer_id);
    notification_message_observer_peer_ids_.erase(peer_id);

    if (multitouch_extension_peer_id_ == peer_id) {
      logger::get_logger()->debug("multitouch_extension is closed.");

      multitouch_extension_peer_id_ = std::nullopt;
      clear_multitouch_extension_environment_variables();
    }

    if (core_service_agent_peer_id_ == peer_id) {
      core_service_agent_peer_id_ = std::nullopt;
    }

    if (console_user_server_peer_id_ == peer_id) {
      console_user_server_peer_id_ = std::nullopt;
      console_user_server_peer_ = nullptr;

      stop_device_grabber();
      start_grabbing_if_system_core_configuration_file_exists();
    }
  }

  void handle_request(const xpc::request_context& context,
                      std::shared_ptr<const std::vector<uint8_t>> buffer) {
    auto peer_id = context.peer.id;
    auto reply = context.reply;

    if (buffer->empty()) {
      server_->async_cancel_peer(peer_id);
      return;
    }

    if (!context.peer.identity) {
      server_->async_cancel_peer(peer_id);
      return;
    }

    auto peer_euid = context.peer.identity->get_euid();

    try {
      nlohmann::json json = nlohmann::json::from_msgpack(*buffer);

      auto operation_type_value = json.at("operation_type").get<operation_type>();
      if (operation_type_value == operation_type::console_user_id_changed) {
        auto uid = peer_euid;

        if (!json.at("on_console").get<bool>()) {
          // A Console User Server for a user who is no longer on the console can
          // remain connected while another account is active. Only clear the
          // console UID when the notification is for the active account.
          if (current_console_user_id_ == uid) {
            console_user_id_changed(std::nullopt);
          }
          async_respond_none(reply);
          return;
        }

        if (current_console_user_id_ != uid) {
          // A user has become the console user, or the console account changed.
          // Recreate the listener with this UID before accepting ordinary
          // requests. Cancel this peer so the Console User Server reconnects
          // and repeats its notification to the replacement listener.
          console_user_id_changed(uid);
          server_->async_cancel_peer(peer_id);
          return;
        }

        // The replacement listener is ready for this console account. Return
        // the bound state so the Console User Server can start its regular
        // CoreService daemon client connection.
        async_respond(reply,
                      nlohmann::json{
                          {"operation_type", operation_type::core_service_daemon_server_bound},
                          {"uid", uid},
                      });
        return;
      }

      if (!current_console_user_id_ ||
          peer_euid != *current_console_user_id_) {
        server_->async_cancel_peer(peer_id);
        return;
      }

      switch (operation_type_value) {
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

              process_lifecycle_manager::async_request_termination();
            }
          }
          async_respond_none(reply);
          break;

        case operation_type::system_preferences_updated:
          system_preferences_properties_ = json.at("system_preferences_properties")
                                               .get<pqrs::osx::system_preferences::properties>();
          if (device_grabber_) {
            device_grabber_->async_set_system_preferences_properties(system_preferences_properties_);
          }
          logger::get_logger()->debug("`system_preferences` is updated.");
          async_respond_none(reply);
          break;

        case operation_type::start_device_grabber: {
          console_user_server_peer_id_ = peer_id;
          console_user_server_peer_ = std::make_shared<console_user_server_peer>(weak_dispatcher_,
                                                                                 server_,
                                                                                 peer_id);

          stop_device_grabber();
          start_device_grabber(json.at("user_core_configuration_file_path").get<std::string>());

          if (auto m = weak_core_service_daemon_state_manager_.lock()) {
            send_core_service_daemon_state(m->copy_state());
          }

          request_core_service_bundle_permission_check_result_refresh();

          async_respond_none(reply);
          break;
        }

        case operation_type::frontmost_application_changed: {
          auto app = json.at("frontmost_application").get<application>();

          //
          // Synchronize frontmost_application_changed to console_user_server
          //

          if (console_user_server_peer_) {
            console_user_server_peer_->async_frontmost_application_changed(app);
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
          async_respond_none(reply);
          break;
        }

        case operation_type::focused_ui_element_changed: {
          auto element = json.at("focused_ui_element").get<focused_ui_element>();

          //
          // Synchronize focused_ui_element_changed to console_user_server
          //

          if (console_user_server_peer_) {
            console_user_server_peer_->async_focused_ui_element_changed(element);
          }

          //
          // Update accessibility.* variables
          //

          focused_ui_element_ = element;
          set_focused_ui_element_variables();
          async_respond_none(reply);
          break;
        }

        case operation_type::input_source_changed:
          input_source_properties_ = json.at("input_source_properties")
                                         .get<pqrs::osx::input_source::properties>();
          if (device_grabber_) {
            device_grabber_->async_post_input_source_changed_event(input_source_properties_);
          }
          async_respond_none(reply);
          break;

        case operation_type::temporarily_ignore_device: {
          const auto& device_id_json = json.at("device_id");

          // null means that EventViewer no longer requests a device to be ignored,
          // such as when capture stops or MainView captures All devices.
          if (device_id_json.is_null()) {
            temporarily_ignored_device_ids_by_peer_.erase(peer_id);
          } else {
            temporarily_ignored_device_ids_by_peer_.insert_or_assign(
                peer_id,
                device_id(device_id_json.get<uint64_t>()));
          }

          update_temporarily_ignored_device_ids();
          async_respond_none(reply);
          break;
        }

        case operation_type::get_manipulator_environment:
          if (device_grabber_) {
            device_grabber_->async_invoke_with_manipulator_environment(
                [this, reply](auto&& manipulator_environment) {
                  async_respond(reply,
                                nlohmann::json{
                                    {"operation_type", operation_type::manipulator_environment},
                                    {"manipulator_environment", manipulator_environment.to_json()},
                                });
                });
          } else {
            async_respond_none(reply);
          }
          break;

        case operation_type::connect_multitouch_extension:
          logger::get_logger()->debug("multitouch_extension is connected.");
          multitouch_extension_peer_id_ = peer_id;
          async_respond_none(reply);
          break;

        case operation_type::set_app_icon: {
          // `set_app_icon` requires root privileges.
          auto number = json.at("number").get<int>();

          logger::get_logger()->info("set_app_icon {0}", number);

          app_icon(number).save_to_file(constants::get_system_app_icon_configuration_file_path());

          application_launcher::launch_app_icon_switcher();

          if (console_user_server_peer_) {
            console_user_server_peer_->async_app_icon_changed(number);
          }

          async_respond_none(reply);
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
          async_respond_none(reply);
          break;

        case operation_type::clear_user_variables:
          if (device_grabber_) {
            device_grabber_->async_invoke_with_manipulator_environment(
                [this, reply](auto&& manipulator_environment) {
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

                  async_respond_none(reply);
                });
          } else {
            async_respond_none(reply);
          }
          break;

        case operation_type::observe_connected_devices:
          connected_devices_observer_peer_ids_.insert(peer_id);
          async_respond_connected_devices(reply);
          break;

        case operation_type::observe_notification_message:
          notification_message_observer_peer_ids_.insert(peer_id);
          async_respond_notification_message(reply);
          break;

        case operation_type::get_system_variables:
          if (device_grabber_) {
            device_grabber_->async_invoke_with_manipulator_environment(
                [this, reply](auto&& manipulator_environment) {
                  async_respond(
                      reply,
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
                                      "system.ui_language",
                                      manipulator_environment.get_variable("system.ui_language"),
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
            async_respond_none(reply);
          }
          break;

        case operation_type::get_multitouch_extension_variables:
          if (device_grabber_) {
            device_grabber_->async_invoke_with_manipulator_environment(
                [this, reply](auto&& manipulator_environment) {
                  std::unordered_map<std::string, manipulator_environment_variable_value> variables;
                  for (const auto& name : multitouch_extension_environment_variable_names) {
                    variables[name] = manipulator_environment.get_variable(name);
                  }

                  async_respond(reply,
                                nlohmann::json{
                                    {"operation_type", operation_type::multitouch_extension_variables},
                                    {"multitouch_extension_variables", variables}});
                });
          } else {
            async_respond_none(reply);
          }
          break;

        default:
          server_->async_cancel_peer(peer_id);
          break;
      }
    } catch (std::exception& e) {
      logger::get_logger()->error("received data is corrupted");
      server_->async_cancel_peer(peer_id);
    }
  }

  void start_grabbing_if_system_core_configuration_file_exists() {
    if (!required_permissions_granted()) {
      logger::get_logger()->info("device_grabber is not started because the required permissions are not granted.");
      return;
    }

    auto file_path = constants::get_system_core_configuration_file_path();
    if (filesystem_utility::exists(file_path)) {
      stop_device_grabber();
      start_device_grabber(std::nullopt);
    }
  }

  void start_device_grabber(const std::optional<std::string>& user_core_configuration_file_path) {
    if (device_grabber_) {
      return;
    }

    if (!required_permissions_granted()) {
      logger::get_logger()->info("device_grabber is not started because the required permissions are not granted.");
      return;
    }

    if (auto m = weak_core_service_daemon_state_manager_.lock()) {
      m->reset_device_grabber_state();
    }

    device_grabber_ = std::make_unique<device_grabber>(console_user_server_peer_,
                                                       weak_core_service_daemon_state_manager_);

    connected_devices_changed_connection_ = device_grabber_->connected_devices_changed.connect([this] {
      send_connected_devices_to_observers();
    });

    notification_message_changed_connection_ = device_grabber_->notification_message_changed.connect([this](const auto&) {
      send_notification_message_to_observers();
    });

    device_grabber_->async_set_system_preferences_properties(system_preferences_properties_);
    device_grabber_->async_post_frontmost_application_changed_event(frontmost_application_);
    set_focused_ui_element_variables();
    device_grabber_->async_post_input_source_changed_event(input_source_properties_);
    update_temporarily_ignored_device_ids();

    device_grabber_->async_start(user_core_configuration_file_path,
                                 current_console_user_id_);

    logger::get_logger()->debug("device_grabber is started.");
  }

  void stop_device_grabber() {
    if (!device_grabber_) {
      return;
    }

    connected_devices_changed_connection_.disconnect();
    notification_message_changed_connection_.disconnect();
    device_grabber_ = nullptr;
    if (auto m = weak_core_service_daemon_state_manager_.lock()) {
      m->reset_device_grabber_state();
    }

    send_connected_devices_to_observers();
    send_notification_message_to_observers();

    logger::get_logger()->debug("device_grabber is stopped.");
  }

  void update_temporarily_ignored_device_ids() {
    if (!device_grabber_) {
      return;
    }

    std::unordered_set<device_id> device_ids;
    for (const auto& device_id : temporarily_ignored_device_ids_by_peer_ | std::views::values) {
      device_ids.insert(device_id);
    }

    device_grabber_->async_set_temporarily_ignored_device_ids(device_ids);
  }

  [[nodiscard]] bool required_permissions_granted() const {
    if (auto m = weak_core_service_daemon_state_manager_.lock()) {
      if (auto result = m->copy_current_process_permission_check_result()) {
        return result->required_permissions_granted();
      }
    }

    return false;
  }

  void async_respond_connected_devices(xpc::reply_token reply) {
    async_invoke_with_connected_devices_message(
        [this, reply](const auto& message) {
          async_respond(reply,
                        message);
        });
  }

  void send_connected_devices_to_observers() {
    async_invoke_with_connected_devices_message(
        [this](const auto& message) {
          for (const auto& peer_id : connected_devices_observer_peer_ids_) {
            async_request(peer_id,
                          message);
          }
        });
  }

  void async_invoke_with_connected_devices_message(std::function<void(const nlohmann::json&)> function) {
    auto invoke = [function](const auto& connected_devices_json) {
      function(nlohmann::json{
          {"operation_type", operation_type::connected_devices},
          {"connected_devices", connected_devices_json},
      });
    };

    if (device_grabber_) {
      device_grabber_->async_invoke_with_connected_devices(invoke);
    } else {
      invoke(nlohmann::json::array());
    }
  }

  void async_respond_notification_message(xpc::reply_token reply) {
    async_invoke_with_notification_message_message(
        [this, reply](const auto& message) {
          async_respond(reply,
                        message);
        });
  }

  void send_notification_message_to_observers() {
    async_invoke_with_notification_message_message(
        [this](const auto& message) {
          for (const auto& peer_id : notification_message_observer_peer_ids_) {
            async_request(peer_id,
                          message);
          }
        });
  }

  void async_invoke_with_notification_message_message(std::function<void(const nlohmann::json&)> function) {
    auto invoke = [function](const auto& notification_message) {
      function(nlohmann::json{
          {"operation_type", operation_type::notification_message},
          {"notification_message", notification_message},
      });
    };

    if (device_grabber_) {
      device_grabber_->async_invoke_with_notification_message(invoke);
    } else {
      invoke("");
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
              "accessibility.focused_ui_element.window_title_string",
              manipulator_environment_variable_value(focused_ui_element_.get_window_title().value_or("")),
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
    if (console_user_server_peer_) {
      console_user_server_peer_->async_core_service_daemon_state(core_service_daemon_state);
    }
  }

  void request_core_service_bundle_permission_check_result_refresh() {
    if (!core_service_agent_peer_id_) {
      return;
    }

    async_request(
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

  std::shared_ptr<xpc::listener> server_;
  std::unique_ptr<device_grabber> device_grabber_;
  nod::scoped_connection connected_devices_changed_connection_;
  nod::scoped_connection notification_message_changed_connection_;
  std::optional<xpc::peer_id> core_service_agent_peer_id_;
  std::optional<xpc::peer_id> console_user_server_peer_id_;
  std::shared_ptr<console_user_server_peer> console_user_server_peer_;
  std::optional<xpc::peer_id> multitouch_extension_peer_id_;
  std::unordered_map<xpc::peer_id, device_id> temporarily_ignored_device_ids_by_peer_;
  std::unordered_set<xpc::peer_id> connected_devices_observer_peer_ids_;
  std::unordered_set<xpc::peer_id> notification_message_observer_peer_ids_;

  pqrs::osx::system_preferences::properties system_preferences_properties_;
  application frontmost_application_;
  focused_ui_element focused_ui_element_;
  pqrs::osx::input_source::properties input_source_properties_;
};
} // namespace krbn::core_service::daemon
