#pragma once

// `krbn::core_service::daemon::receiver` can be used safely in a multi-threaded environment.

#include "app_icon.hpp"
#include "application_launcher.hpp"
#include "console_user_server_client.hpp"
#include "constants.hpp"
#include "core_service/daemon/core_service_state_json_writer.hpp"
#include "device_grabber.hpp"
#include "filesystem_utility.hpp"
#include "shared_secret_authentication.hpp"
#include "types.hpp"
#include <array>
#include <pqrs/dispatcher.hpp>
#include <pqrs/local_datagram.hpp>
#include <pqrs/osx/session.hpp>
#include <pqrs/osx/system_preferences.hpp>
#include <pqrs/osx/system_preferences/extra/nlohmann_json.hpp>
#include <vector>

namespace krbn {
namespace core_service {
namespace daemon {
class receiver final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  receiver(const receiver&) = delete;

  receiver(std::optional<uid_t> current_console_user_id,
           std::weak_ptr<core_service_state_json_writer> weak_core_service_state_json_writer)
      : dispatcher_client(),
        current_console_user_id_(current_console_user_id),
        weak_core_service_state_json_writer_(weak_core_service_state_json_writer) {
    // Remove old files and prepare a socket directory.
    prepare_karabiner_core_service_socket_directory();

    shared_secret_authentication_receiver_ =
        std::make_unique<shared_secret_authentication::receiver>(weak_dispatcher_,
                                                                 constants::local_datagram_buffer_size);

    event_viewer_temporarily_ignore_all_devices_peer_manager_ = std::make_unique<pqrs::local_datagram::extra::peer_manager>(
        weak_dispatcher_,
        constants::local_datagram_buffer_size,
        [](auto&& peer_pid,
           auto&& peer_socket_file_path) {
          return true;
        });
    event_viewer_temporarily_ignore_all_devices_peer_manager_->peer_closed.connect(
        [this](auto&& peer_socket_file_path,
               auto&& remaining_verified_peer_count) {
          // Restore the flags when all clients have closed.
          if (remaining_verified_peer_count == 0) {
            if (device_grabber_) {
              device_grabber_->async_set_temporarily_ignore_all_devices(false);
            }
          }
        });

    //
    // Setup server_
    //

    auto socket_file_path = karabiner_core_service_socket_file_path();

    server_ = std::make_unique<pqrs::local_datagram::server>(weak_dispatcher_,
                                                             socket_file_path,
                                                             constants::local_datagram_buffer_size);
    server_->set_server_check_interval(std::chrono::milliseconds(3000));
    server_->set_reconnect_interval(std::chrono::milliseconds(1000));

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

    server_->received.connect([this](auto&& buffer, auto&& sender_endpoint) {
      if (buffer->empty()) {
        return;
      }

      try {
        nlohmann::json json = nlohmann::json::from_msgpack(*buffer);
        auto ot = json.at("operation_type").get<operation_type>();
        if (ot == operation_type::handshake) {
          if (shared_secret_authentication_receiver_) {
            shared_secret_authentication_receiver_->handle_handshake(sender_endpoint->path());
          }
          return;
        }

        if (!shared_secret_authentication_receiver_ ||
            !shared_secret_authentication_receiver_->verify_shared_secret(sender_endpoint->path(),
                                                                          json,
                                                                          ot)) {
          return;
        }

        switch (ot) {
          case operation_type::system_preferences_updated:
            system_preferences_properties_ = json.at("system_preferences_properties")
                                                 .get<pqrs::osx::system_preferences::properties>();
            if (device_grabber_) {
              device_grabber_->async_set_system_preferences_properties(system_preferences_properties_);
            }
            logger::get_logger()->info("`system_preferences` is updated.");
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
            break;
          }

          case operation_type::focused_ui_element_changed: {
            auto element = json.at("focused_ui_element").get<focused_ui_element>();

            focused_ui_element_ = element;
            set_focused_ui_element_variables();
            break;
          }

          case operation_type::input_source_changed:
            input_source_properties_ = json.at("input_source_properties")
                                           .get<pqrs::osx::input_source::properties>();
            if (device_grabber_) {
              device_grabber_->async_post_input_source_changed_event(input_source_properties_);
            }
            break;

          case operation_type::temporarily_ignore_all_devices: {
            auto value = json.at("value").get<bool>();

            if (device_grabber_) {
              device_grabber_->async_set_temporarily_ignore_all_devices(value);
            }

            if (event_viewer_temporarily_ignore_all_devices_peer_manager_) {
              // Register the client with peer_manager so the flag is turned off when the client closes.
              nlohmann::json json{
                  {"operation_type", operation_type::none},
              };
              event_viewer_temporarily_ignore_all_devices_peer_manager_->async_send(sender_endpoint->path(),
                                                                                    nlohmann::json::to_msgpack(json));
            }

            break;
          }

          case operation_type::get_manipulator_environment:
            if (device_grabber_) {
              device_grabber_->async_invoke_with_manipulator_environment(
                  [this, sender_endpoint](auto&& manipulator_environment) {
                    if (shared_secret_authentication_receiver_) {
                      shared_secret_authentication_receiver_->async_send(
                          sender_endpoint->path(),
                          nlohmann::json{
                              {"operation_type", operation_type::manipulator_environment},
                              {"manipulator_environment", manipulator_environment.to_json()},
                          });
                    }
                  });
            }
            break;

          case operation_type::connect_multitouch_extension:
            logger::get_logger()->info("multitouch_extension is connected.");

            multitouch_extension_client_ = std::make_unique<pqrs::local_datagram::client>(weak_dispatcher_,
                                                                                          sender_endpoint->path(),
                                                                                          std::nullopt,
                                                                                          constants::local_datagram_buffer_size);
            multitouch_extension_client_->set_server_check_interval(std::chrono::milliseconds(3000));
            multitouch_extension_client_->set_client_socket_check_interval(std::chrono::milliseconds(3000));

            multitouch_extension_client_->connected.connect([](auto&& peer_pid) {
              logger::get_logger()->info("multitouch_extension_client_->connected");
            });

            multitouch_extension_client_->connect_failed.connect([this](auto&& error_code) {
              logger::get_logger()->info("multitouch_extension_client_->connect_failed");

              multitouch_extension_client_ = nullptr;
              clear_multitouch_extension_environment_variables();
            });

            multitouch_extension_client_->closed.connect([this] {
              logger::get_logger()->info("multitouch_extension_client_->closed");

              multitouch_extension_client_ = nullptr;
              clear_multitouch_extension_environment_variables();
            });

            multitouch_extension_client_->error_occurred.connect([](auto&& error_code) {
              logger::get_logger()->error("multitouch_extension_client_->error_occurred: {0}", error_code.message());
            });

            multitouch_extension_client_->async_start();

            break;

          case operation_type::set_app_icon: {
            // `set_app_icon` requires root privileges.
            auto number = json.at("number").get<int>();

            logger::get_logger()->info("set_app_icon {0}", number);

            app_icon(number).async_save_to_file(constants::get_system_app_icon_configuration_file_path());

            application_launcher::launch_app_icon_switcher();

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
            break;

          case operation_type::clear_user_variables:
            if (device_grabber_) {
              device_grabber_->async_invoke_with_manipulator_environment(
                  [this, sender_endpoint](auto&& manipulator_environment) {
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
                  });
            }
            break;

          case operation_type::get_connected_devices:
            if (device_grabber_) {
              device_grabber_->async_invoke_with_connected_devices(
                  [this, sender_endpoint](auto&& connected_devices_json) {
                    if (shared_secret_authentication_receiver_) {
                      shared_secret_authentication_receiver_->async_send(
                          sender_endpoint->path(),
                          nlohmann::json{
                              {"operation_type", operation_type::connected_devices},
                              {"connected_devices", connected_devices_json},
                          });
                    }
                  });
            }
            break;

          case operation_type::get_notification_message:
            if (device_grabber_) {
              device_grabber_->async_invoke_with_notification_message(
                  [this, sender_endpoint](auto&& message) {
                    if (shared_secret_authentication_receiver_) {
                      shared_secret_authentication_receiver_->async_send(
                          sender_endpoint->path(),
                          nlohmann::json{
                              {"operation_type", operation_type::notification_message},
                              {"notification_message", message},
                          });
                    }
                  });
            }
            break;

          case operation_type::get_system_variables:
            if (device_grabber_) {
              device_grabber_->async_invoke_with_manipulator_environment(
                  [this, sender_endpoint](auto&& manipulator_environment) {
                    if (shared_secret_authentication_receiver_) {
                      shared_secret_authentication_receiver_->async_send(
                          sender_endpoint->path(),
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
                    }
                  });
            }
            break;

          case operation_type::get_multitouch_extension_variables:
            if (device_grabber_) {
              device_grabber_->async_invoke_with_manipulator_environment(
                  [this, sender_endpoint](auto&& manipulator_environment) {
                    std::unordered_map<std::string, manipulator_environment_variable_value> variables;
                    for (const auto& name : multitouch_extension_environment_variable_names) {
                      variables[name] = manipulator_environment.get_variable(name);
                    }

                    if (shared_secret_authentication_receiver_) {
                      shared_secret_authentication_receiver_->async_send(
                          sender_endpoint->path(),
                          nlohmann::json{
                              {"operation_type", operation_type::multitouch_extension_variables},
                              {"multitouch_extension_variables", variables}});
                    }
                  });
            }
            break;

          default:
            break;
        }
      } catch (std::exception& e) {
        logger::get_logger()->error("received data is corrupted");
      }
    });

    server_->async_start();

    //
    // Setup console_user_server_client_
    //

    if (current_console_user_id_) {
      console_user_server_client_ = std::make_unique<console_user_server_client>(*current_console_user_id_,
                                                                                 "cs_con_usr_srv_clnt");

      console_user_server_client_->connected.connect([this] {
        console_user_server_client_->async_get_user_core_configuration_file_path();
      });

      // If the console_user_server isn't running (i.e., operating under system_core_configuration),
      // connect_failed will keep being called, so we won't perform any operations on the device_grabber.
      console_user_server_client_->connect_failed.connect([this](auto&& error_code) {
        filesystem_utility::create_base_directories(current_console_user_id_);
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

    logger::get_logger()->info("receiver is initialized");
  }

  virtual ~receiver() {
    detach_from_dispatcher([this] {
      server_ = nullptr;
      console_user_server_client_ = nullptr;
      multitouch_extension_client_ = nullptr;
      shared_secret_authentication_receiver_ = nullptr;
      event_viewer_temporarily_ignore_all_devices_peer_manager_ = nullptr;
      stop_device_grabber();
    });

    logger::get_logger()->info("receiver is terminated");
  }

private:
  std::filesystem::path karabiner_core_service_socket_file_path() const {
    return constants::get_karabiner_core_service_socket_directory_path() / filesystem_utility::make_socket_file_basename();
  }

  void prepare_karabiner_core_service_socket_directory() const {
    filesystem_utility::create_base_directories(current_console_user_id_);

    auto directory_path = constants::get_karabiner_core_service_socket_directory_path();
    std::error_code ec;
    std::filesystem::remove_all(directory_path, ec);
    std::filesystem::create_directory(directory_path, ec);
    chmod(directory_path.c_str(), 0755);
  }

  void start_grabbing_if_system_core_configuration_file_exists() {
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

    device_grabber_ = std::make_unique<device_grabber>(console_user_server_client_,
                                                       weak_core_service_state_json_writer_);

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

    logger::get_logger()->info("device_grabber is stopped.");
  }

  void set_focused_ui_element_variables() {
    if (device_grabber_) {
      device_grabber_->async_post_set_variable_event(
          manipulator_environment_variable_set_variable(
              "accessibility.focused_ui_element.role",
              manipulator_environment_variable_value(focused_ui_element_.get_role().value_or("")),
              nullptr,
              std::nullopt,
              nullptr));

      device_grabber_->async_post_set_variable_event(
          manipulator_environment_variable_set_variable(
              "accessibility.focused_ui_element.subrole",
              manipulator_environment_variable_value(focused_ui_element_.get_subrole().value_or("")),
              nullptr,
              std::nullopt,
              nullptr));

      device_grabber_->async_post_set_variable_event(
          manipulator_environment_variable_set_variable(
              "accessibility.focused_ui_element.title",
              manipulator_environment_variable_value(focused_ui_element_.get_title().value_or("")),
              nullptr,
              std::nullopt,
              nullptr));
    }
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
  std::weak_ptr<core_service_state_json_writer> weak_core_service_state_json_writer_;

  std::unique_ptr<pqrs::local_datagram::extra::peer_manager> event_viewer_temporarily_ignore_all_devices_peer_manager_;
  std::unique_ptr<shared_secret_authentication::receiver> shared_secret_authentication_receiver_;
  std::unique_ptr<pqrs::local_datagram::server> server_;
  std::shared_ptr<console_user_server_client> console_user_server_client_;
  std::unique_ptr<pqrs::local_datagram::client> multitouch_extension_client_;
  std::unique_ptr<device_grabber> device_grabber_;

  pqrs::osx::system_preferences::properties system_preferences_properties_;
  application frontmost_application_;
  focused_ui_element focused_ui_element_;
  pqrs::osx::input_source::properties input_source_properties_;
};
} // namespace daemon
} // namespace core_service
} // namespace krbn
