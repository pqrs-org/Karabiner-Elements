#pragma once

// `krbn::grabber::receiver` can be used safely in a multi-threaded environment.

#include "console_user_server_client.hpp"
#include "constants.hpp"
#include "device_grabber.hpp"
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

  receiver(uid_t current_console_user_id) : dispatcher_client(),
                                            current_console_user_id_(current_console_user_id) {
    std::string socket_file_path(constants::get_grabber_socket_file_path());

    unlink(socket_file_path.c_str());

    server_ = std::make_unique<pqrs::local_datagram::server>(weak_dispatcher_,
                                                             socket_file_path,
                                                             constants::get_local_datagram_buffer_size());
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
      if (buffer) {
        if (buffer->empty()) {
          return;
        }

        try {
          nlohmann::json json = nlohmann::json::from_msgpack(*buffer);
          switch (json.at("operation_type").get<operation_type>()) {
            case operation_type::key_down_up_valued_event_arrived: {
              if (device_grabber_) {
                device_grabber_->async_update_probable_stuck_events_by_observer(
                    json.at("device_id").get<device_id>(),
                    json.at("key_down_up_valued_event").get<key_down_up_valued_event>(),
                    json.at("event_type").get<event_type>(),
                    json.at("time_stamp").get<absolute_time_point>());
              }
              break;
            }

            case operation_type::observed_devices_updated: {
              if (device_grabber_) {
                device_grabber_->async_set_observed_devices(
                    json.at("observed_devices").get<std::unordered_set<device_id>>());
              }
              break;
            }

            case operation_type::caps_lock_state_changed: {
              auto caps_lock_state = json.at("caps_lock_state").get<bool>();
              if (device_grabber_) {
                device_grabber_->async_set_caps_lock_state(caps_lock_state);
              }
              break;
            }

            case operation_type::connect_console_user_server: {
              auto user_core_configuration_file_path =
                  json.at("user_core_configuration_file_path").get<std::string>();

              logger::get_logger()->info("karabiner_console_user_server is connected.");

              console_user_server_client_ = nullptr;
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

              console_user_server_client_->async_start(current_console_user_id_);
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

            case operation_type::set_variables:
              if (device_grabber_) {
                for (const auto& [k, v] : json.at("variables").items()) {
                  device_grabber_->async_post_set_variable_event(k, v.get<int>());
                }
              }
              break;

            default:
              break;
          }
          return;
        } catch (std::exception& e) {
          logger::get_logger()->error("received data is corrupted");
        }
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
      stop_device_grabber();
    });

    logger::get_logger()->info("receiver is terminated");
  }

private:
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

    device_grabber_ = std::make_unique<device_grabber>(console_user_server_client_);

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

  uid_t current_console_user_id_;
  std::unique_ptr<pqrs::local_datagram::server> server_;
  std::shared_ptr<console_user_server_client> console_user_server_client_;
  std::unique_ptr<device_grabber> device_grabber_;

  pqrs::osx::system_preferences::properties system_preferences_properties_;
  pqrs::osx::frontmost_application_monitor::application frontmost_application_;
  pqrs::osx::input_source::properties input_source_properties_;
};
} // namespace grabber
} // namespace krbn
