#pragma once

// `krbn::grabber::receiver` can be used safely in a multi-threaded environment.

#include "app_icon.hpp"
#include "application_launcher.hpp"
#include "console_user_server_client.hpp"
#include "constants.hpp"
#include "device_grabber.hpp"
#include "filesystem_utility.hpp"
#include "grabber/grabber_state_json_writer.hpp"
#include "types.hpp"
#include <pqrs/dispatcher.hpp>
#include <pqrs/local_datagram.hpp>
#include <pqrs/osx/session.hpp>
#include <pqrs/osx/system_preferences.hpp>
#include <pqrs/osx/system_preferences/extra/nlohmann_json.hpp>
#include <vector>

namespace krbn {
namespace grabber {
class receiver final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  receiver(const receiver&) = delete;

  receiver(uid_t current_console_user_id,
           std::weak_ptr<grabber_state_json_writer> weak_grabber_state_json_writer) : dispatcher_client(),
                                                                                      current_console_user_id_(current_console_user_id),
                                                                                      weak_grabber_state_json_writer_(weak_grabber_state_json_writer) {
    // Remove old socket files.
    {
      auto directory_path = constants::get_grabber_socket_directory_path();
      std::error_code ec;
      std::filesystem::remove_all(directory_path, ec);
      std::filesystem::create_directory(directory_path, ec);
      chmod(directory_path.c_str(), 0755);
    }

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

    event_viewer_get_manipulator_environment_peer_manager_ = std::make_unique<pqrs::local_datagram::extra::peer_manager>(
        weak_dispatcher_,
        constants::local_datagram_buffer_size,
        [](auto&& peer_pid,
           auto&& peer_socket_file_path) {
          // Verify the peer's Team ID before sending manipulator environment information.
          if (get_shared_codesign_manager()->same_team_id(peer_pid)) {
            logger::get_logger()->info("verified peer connected");
            return true;
          } else {
            logger::get_logger()->warn("peer is not code-signed with same Team ID");
            return false;
          }
        });

    verification_exempt_peer_manager_ = std::make_unique<pqrs::local_datagram::extra::peer_manager>(
        weak_dispatcher_,
        constants::local_datagram_buffer_size,
        [](auto&& peer_pid,
           auto&& peer_socket_file_path) {
          return true;
        });

    auto socket_file_path = grabber_socket_file_path();

    server_ = std::make_unique<pqrs::local_datagram::server>(weak_dispatcher_,
                                                             socket_file_path,
                                                             constants::local_datagram_buffer_size);
    server_->set_server_check_interval(std::chrono::milliseconds(3000));
    server_->set_reconnect_interval(std::chrono::milliseconds(1000));

    server_->bound.connect([this, socket_file_path] {
      logger::get_logger()->info("receiver: bound");

      logger::get_logger()->info("receiver: chown socket: {0}", current_console_user_id_);
      chown(socket_file_path.c_str(), current_console_user_id_, 0);

      chmod(socket_file_path.c_str(), 0600);
    });

    server_->bind_failed.connect([](auto&& error_code) {
      logger::get_logger()->error("receiver: bind_failed");
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
        switch (json.at("operation_type").get<operation_type>()) {
          case operation_type::connect_console_user_server: {
            auto user_core_configuration_file_path =
                json.at("user_core_configuration_file_path").get<std::string>();

            logger::get_logger()->info("karabiner_console_user_server is connected.");

            console_user_server_client_ = nullptr;

            if (!sender_endpoint->path().empty()) {
              console_user_server_client_ = std::make_shared<console_user_server_client>();

              console_user_server_client_->connected.connect([this, user_core_configuration_file_path] {
                stop_device_grabber();
                start_device_grabber(user_core_configuration_file_path);
              });

              console_user_server_client_->connect_failed.connect([this](auto&& error_code) {
                console_user_server_client_ = nullptr;

                stop_device_grabber();
                start_grabbing_if_system_core_configuration_file_exists();
              });

              console_user_server_client_->closed.connect([this] {
                console_user_server_client_ = nullptr;

                stop_device_grabber();
                start_grabbing_if_system_core_configuration_file_exists();
              });

              console_user_server_client_->async_start(sender_endpoint->path());
            }

            break;
          }

          case operation_type::system_preferences_updated:
            system_preferences_properties_ = json.at("system_preferences_properties")
                                                 .get<pqrs::osx::system_preferences::properties>();
            if (device_grabber_) {
              device_grabber_->async_set_system_preferences_properties(system_preferences_properties_);
            }
            logger::get_logger()->info("`system_preferences` is updated.");
            break;

          case operation_type::frontmost_application_changed:
            frontmost_application_ = json.at("frontmost_application")
                                         .get<pqrs::osx::frontmost_application_monitor::application>();
            if (device_grabber_) {
              device_grabber_->async_post_frontmost_application_changed_event(frontmost_application_);
            }
            break;

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
                    if (event_viewer_get_manipulator_environment_peer_manager_) {
                      nlohmann::json json{
                          {"operation_type", operation_type::manipulator_environment},
                          {"manipulator_environment", manipulator_environment.to_json()},
                      };
                      event_viewer_get_manipulator_environment_peer_manager_->async_send(sender_endpoint->path(),
                                                                                         nlohmann::json::to_msgpack(json));
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
            multitouch_extension_client_->set_next_heartbeat_deadline(std::chrono::milliseconds(10000));
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

          case operation_type::get_system_variables:
            if (device_grabber_) {
              device_grabber_->async_invoke_with_manipulator_environment(
                  [this, sender_endpoint](auto&& manipulator_environment) {
                    if (verification_exempt_peer_manager_) {
                      nlohmann::json json{
                          {"operation_type", operation_type::system_variables},
                          {
                              "system_variables",
                              {
                                  {
                                      "system.use_fkeys_as_standard_function_keys",
                                      manipulator_environment.get_variable("system.use_fkeys_as_standard_function_keys"),
                                  },
                                  {
                                      "system.scroll_direction_is_natural",
                                      manipulator_environment.get_variable("system.scroll_direction_is_natural"),
                                  },
                              },
                          }};
                      verification_exempt_peer_manager_->async_send(sender_endpoint->path(),
                                                                    nlohmann::json::to_msgpack(json));
                    }
                  });
            }
            break;

          default:
            break;
        }
        return;
      } catch (std::exception& e) {
        logger::get_logger()->error("received data is corrupted");
      }
    });

    server_->async_start();

    start_grabbing_if_system_core_configuration_file_exists();

    logger::get_logger()->info("receiver is initialized");
  }

  virtual ~receiver(void) {
    detach_from_dispatcher([this] {
      server_ = nullptr;
      console_user_server_client_ = nullptr;
      multitouch_extension_client_ = nullptr;
      verification_exempt_peer_manager_ = nullptr;
      event_viewer_get_manipulator_environment_peer_manager_ = nullptr;
      event_viewer_temporarily_ignore_all_devices_peer_manager_ = nullptr;
      stop_device_grabber();
    });

    logger::get_logger()->info("receiver is terminated");
  }

private:
  std::filesystem::path grabber_socket_file_path(void) const {
    return constants::get_grabber_socket_directory_path() / filesystem_utility::make_socket_file_basename();
  }

  void start_grabbing_if_system_core_configuration_file_exists(void) {
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
                                                       weak_grabber_state_json_writer_);

    device_grabber_->async_set_system_preferences_properties(system_preferences_properties_);
    device_grabber_->async_post_frontmost_application_changed_event(frontmost_application_);
    device_grabber_->async_post_input_source_changed_event(input_source_properties_);

    device_grabber_->async_start(configuration_file_path,
                                 current_console_user_id_);

    logger::get_logger()->info("device_grabber is started.");
  }

  void stop_device_grabber(void) {
    if (!device_grabber_) {
      return;
    }

    device_grabber_ = nullptr;

    logger::get_logger()->info("device_grabber is stopped.");
  }

  void clear_multitouch_extension_environment_variables(void) {
    if (device_grabber_) {
      for (const auto& name : {
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
           }) {
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

  uid_t current_console_user_id_;
  std::weak_ptr<grabber_state_json_writer> weak_grabber_state_json_writer_;

  std::unique_ptr<pqrs::local_datagram::extra::peer_manager> event_viewer_temporarily_ignore_all_devices_peer_manager_;
  std::unique_ptr<pqrs::local_datagram::extra::peer_manager> event_viewer_get_manipulator_environment_peer_manager_;
  // For operation_type::get_system_variables
  std::unique_ptr<pqrs::local_datagram::extra::peer_manager> verification_exempt_peer_manager_;
  std::unique_ptr<pqrs::local_datagram::server> server_;
  std::shared_ptr<console_user_server_client> console_user_server_client_;
  std::unique_ptr<pqrs::local_datagram::client> multitouch_extension_client_;
  std::unique_ptr<device_grabber> device_grabber_;

  pqrs::osx::system_preferences::properties system_preferences_properties_;
  pqrs::osx::frontmost_application_monitor::application frontmost_application_;
  pqrs::osx::input_source::properties input_source_properties_;
};
} // namespace grabber
} // namespace krbn
