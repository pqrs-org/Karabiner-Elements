#pragma once

// `krbn::receiver` can be used safely in a multi-threaded environment.

#include "console_user_server_client.hpp"
#include "constants.hpp"
#include "device_grabber.hpp"
#include "dispatcher.hpp"
#include "grabbable_state_queues_manager.hpp"
#include "local_datagram/server_manager.hpp"
#include "session.hpp"
#include "types.hpp"
#include <vector>

namespace krbn {
class receiver final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  receiver(const receiver&) = delete;

  receiver(std::weak_ptr<grabbable_state_queues_manager> weak_grabbable_state_queues_manager) : dispatcher_client(),
                                                                                                weak_grabbable_state_queues_manager_(weak_grabbable_state_queues_manager) {
    std::string socket_file_path(constants::get_grabber_socket_file_path());

    unlink(socket_file_path.c_str());

    size_t buffer_size = 32 * 1024;
    std::chrono::milliseconds server_check_interval(3000);
    std::chrono::milliseconds reconnect_interval(1000);

    server_manager_ = std::make_unique<local_datagram::server_manager>(socket_file_path,
                                                                       buffer_size,
                                                                       server_check_interval,
                                                                       reconnect_interval);

    server_manager_->bound.connect([this, socket_file_path] {
      if (auto uid = session::get_current_console_user_id()) {
        chown(socket_file_path.c_str(), *uid, 0);
      }
      chmod(socket_file_path.c_str(), 0600);

      if (auto m = weak_grabbable_state_queues_manager_.lock()) {
        m->clear();
      }
    });

    server_manager_->bind_failed.connect([](auto&& error_code) {
      logger::get_logger().error("receiver bind_failed");
    });

    server_manager_->received.connect([this](auto&& buffer) {
      if (auto type = types::find_operation_type(*buffer)) {
        switch (*type) {
          case operation_type::grabbable_state_changed:
            if (buffer->size() != sizeof(operation_type_grabbable_state_changed_struct)) {
              logger::get_logger().error("Invalid size for `operation_type::grabbable_state_changed`.");
            } else {
              auto p = reinterpret_cast<operation_type_grabbable_state_changed_struct*>(&((*buffer)[0]));

              if (auto m = weak_grabbable_state_queues_manager_.lock()) {
                m->update_grabbable_state(p->grabbable_state);
              }
            }
            break;

          case operation_type::caps_lock_state_changed:
            if (buffer->size() != sizeof(operation_type_caps_lock_state_changed_struct)) {
              logger::get_logger().error("Invalid size for `operation_type::caps_lock_state_changed`.");
            } else {
              auto p = reinterpret_cast<operation_type_caps_lock_state_changed_struct*>(&((*buffer)[0]));

              if (device_grabber_) {
                device_grabber_->async_set_caps_lock_state(p->state);
              }
            }
            break;

          case operation_type::connect_console_user_server:
            if (buffer->size() != sizeof(operation_type_connect_console_user_server_struct)) {
              logger::get_logger().error("Invalid size for `operation_type::connect_console_user_server`.");
            } else {
              auto p = reinterpret_cast<operation_type_connect_console_user_server_struct*>(&((*buffer)[0]));

              // Ensure user_core_configuration_file_path is null-terminated string even if corrupted data is sent.
              p->user_core_configuration_file_path[sizeof(p->user_core_configuration_file_path) - 1] = '\0';
              std::string user_core_configuration_file_path(p->user_core_configuration_file_path);

              logger::get_logger().info("karabiner_console_user_server is connected (pid:{0})", p->pid);

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

              console_user_server_client_->async_start();
            }
            break;

          case operation_type::system_preferences_updated:
            if (buffer->size() < sizeof(operation_type_system_preferences_updated_struct)) {
              logger::get_logger().error("Invalid size for `operation_type::system_preferences_updated`.");
            } else {
              auto p = reinterpret_cast<operation_type_system_preferences_updated_struct*>(&((*buffer)[0]));

              system_preferences_ = p->system_preferences;

              if (device_grabber_) {
                device_grabber_->async_set_system_preferences(p->system_preferences);
              }

              logger::get_logger().info("`system_preferences` is updated.");
            }
            break;

          case operation_type::frontmost_application_changed:
            if (buffer->size() < sizeof(operation_type_frontmost_application_changed_struct)) {
              logger::get_logger().error("Invalid size for `operation_type::frontmost_application_changed`.");
            } else {
              auto p = reinterpret_cast<operation_type_frontmost_application_changed_struct*>(&((*buffer)[0]));

              // Ensure bundle_identifier and file_path are null-terminated string even if corrupted data is sent.
              p->bundle_identifier[sizeof(p->bundle_identifier) - 1] = '\0';
              p->file_path[sizeof(p->file_path) - 1] = '\0';

              frontmost_application_bundle_identifier_ = p->bundle_identifier;
              frontmost_application_file_path_ = p->file_path;

              if (device_grabber_) {
                device_grabber_->async_post_frontmost_application_changed_event(frontmost_application_bundle_identifier_,
                                                                                frontmost_application_file_path_);
              }
            }
            break;

          case operation_type::input_source_changed:
            if (buffer->size() < sizeof(operation_type_input_source_changed_struct)) {
              logger::get_logger().error("Invalid size for `operation_type::input_source_changed`.");
            } else {
              auto p = reinterpret_cast<operation_type_input_source_changed_struct*>(&((*buffer)[0]));

              // Ensure bundle_identifier and file_path are null-terminated string even if corrupted data is sent.
              p->language[sizeof(p->language) - 1] = '\0';
              p->input_source_id[sizeof(p->input_source_id) - 1] = '\0';
              p->input_mode_id[sizeof(p->input_mode_id) - 1] = '\0';

              input_source_identifiers_ = input_source_identifiers(std::string(p->language),
                                                                   std::string(p->input_source_id),
                                                                   std::string(p->input_mode_id));

              if (device_grabber_) {
                device_grabber_->async_post_input_source_changed_event(input_source_identifiers_);
              }
            }
            break;

          default:
            break;
        }
      }
    });

    server_manager_->async_start();

    start_grabbing_if_system_core_configuration_file_exists();

    logger::get_logger().info("receiver is initialized");
  }

  virtual ~receiver(void) {
    detach_from_dispatcher([this] {
      server_manager_ = nullptr;
      console_user_server_client_ = nullptr;
      stop_device_grabber();
    });

    logger::get_logger().info("receiver is terminated");
  }

private:
  void start_grabbing_if_system_core_configuration_file_exists(void) {
    auto file_path = constants::get_system_core_configuration_file_path();
    if (filesystem::exists(file_path)) {
      stop_device_grabber();
      start_device_grabber(file_path);
    }
  }

  void start_device_grabber(const std::string& configuration_file_path) {
    if (device_grabber_) {
      return;
    }

    device_grabber_ = std::make_unique<device_grabber>(weak_grabbable_state_queues_manager_,
                                                       console_user_server_client_);

    device_grabber_->async_set_system_preferences(system_preferences_);
    device_grabber_->async_post_frontmost_application_changed_event(frontmost_application_bundle_identifier_,
                                                                    frontmost_application_file_path_);
    device_grabber_->async_post_input_source_changed_event(input_source_identifiers_);

    device_grabber_->async_start(configuration_file_path);

    logger::get_logger().info("device_grabber is started.");
  }

  void stop_device_grabber(void) {
    if (!device_grabber_) {
      return;
    }

    device_grabber_ = nullptr;

    logger::get_logger().info("device_grabber is stopped.");
  }

  std::weak_ptr<grabbable_state_queues_manager> weak_grabbable_state_queues_manager_;

  std::unique_ptr<local_datagram::server_manager> server_manager_;
  std::shared_ptr<console_user_server_client> console_user_server_client_;
  std::unique_ptr<device_grabber> device_grabber_;

  system_preferences system_preferences_;
  std::string frontmost_application_bundle_identifier_;
  std::string frontmost_application_file_path_;
  input_source_identifiers input_source_identifiers_;
};
} // namespace krbn
