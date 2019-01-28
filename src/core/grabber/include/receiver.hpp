#pragma once

// `krbn::receiver` can be used safely in a multi-threaded environment.

#include "console_user_server_client.hpp"
#include "constants.hpp"
#include "device_grabber.hpp"
#include "grabbable_state_queues_manager.hpp"
#include "types.hpp"
#include <pqrs/dispatcher.hpp>
#include <pqrs/local_datagram.hpp>
#include <pqrs/osx/session.hpp>
#include <vector>

namespace krbn {
class receiver final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(void)> bound;
  nod::signal<void(const asio::error_code&)> bind_failed;
  nod::signal<void(void)> closed;

  // Methods

  receiver(const receiver&) = delete;

  receiver(std::weak_ptr<grabbable_state_queues_manager> weak_grabbable_state_queues_manager) : dispatcher_client(),
                                                                                                weak_grabbable_state_queues_manager_(weak_grabbable_state_queues_manager) {
    std::string socket_file_path(constants::get_grabber_socket_file_path());

    unlink(socket_file_path.c_str());

    size_t buffer_size = 32 * 1024;
    server_ = std::make_unique<pqrs::local_datagram::server>(weak_dispatcher_,
                                                             socket_file_path,
                                                             buffer_size);
    server_->set_server_check_interval(std::chrono::milliseconds(3000));
    server_->set_reconnect_interval(std::chrono::milliseconds(1000));

    server_->bound.connect([this, socket_file_path] {
      if (auto uid = pqrs::osx::session::find_console_user_id()) {
        chown(socket_file_path.c_str(), *uid, 0);
      }
      chmod(socket_file_path.c_str(), 0600);

      if (auto m = weak_grabbable_state_queues_manager_.lock()) {
        m->clear();
      }
    });

    server_->bind_failed.connect([](auto&& error_code) {
      logger::get_logger()->error("receiver bind_failed");
    });

    server_->received.connect([this](auto&& buffer) {
      if (buffer) {
        if (buffer->empty()) {
          return;
        }

        try {
          nlohmann::json json = nlohmann::json::from_msgpack(*buffer);
          switch (json.at("operation_type").get<operation_type>()) {
            case operation_type::frontmost_application_changed:
              frontmost_application_ = json.at("frontmost_application").get<pqrs::osx::frontmost_application_monitor::application>();
              if (device_grabber_) {
                device_grabber_->async_post_frontmost_application_changed_event(frontmost_application_);
              }
              break;

            case operation_type::input_source_changed:
              input_source_properties_ = json.at("input_source_properties").get<pqrs::osx::input_source::properties>();
              if (device_grabber_) {
                device_grabber_->async_post_input_source_changed_event(input_source_properties_);
              }
              break;

            default:
              break;
          }
          return;
        } catch (std::exception& e) {
          //          logger::get_logger()->error("Received data is corrupted: {0}", e.what());
        }

        if (auto type = types::find_operation_type(*buffer)) {
          switch (*type) {
            case operation_type::grabbable_state_changed:
              if (buffer->size() != sizeof(operation_type_grabbable_state_changed_struct)) {
                logger::get_logger()->error("Invalid size for `operation_type::grabbable_state_changed`.");
              } else {
                auto p = reinterpret_cast<operation_type_grabbable_state_changed_struct*>(&((*buffer)[0]));

                if (auto m = weak_grabbable_state_queues_manager_.lock()) {
                  m->update_grabbable_state(p->grabbable_state);
                }
              }
              break;

            case operation_type::caps_lock_state_changed:
              if (buffer->size() != sizeof(operation_type_caps_lock_state_changed_struct)) {
                logger::get_logger()->error("Invalid size for `operation_type::caps_lock_state_changed`.");
              } else {
                auto p = reinterpret_cast<operation_type_caps_lock_state_changed_struct*>(&((*buffer)[0]));

                if (device_grabber_) {
                  device_grabber_->async_set_caps_lock_state(p->state);
                }
              }
              break;

            case operation_type::connect_console_user_server:
              if (buffer->size() != sizeof(operation_type_connect_console_user_server_struct)) {
                logger::get_logger()->error("Invalid size for `operation_type::connect_console_user_server`.");
              } else {
                auto p = reinterpret_cast<operation_type_connect_console_user_server_struct*>(&((*buffer)[0]));

                // Ensure user_core_configuration_file_path is null-terminated string even if corrupted data is sent.
                p->user_core_configuration_file_path[sizeof(p->user_core_configuration_file_path) - 1] = '\0';
                std::string user_core_configuration_file_path(p->user_core_configuration_file_path);

                logger::get_logger()->info("karabiner_console_user_server is connected (pid:{0})", p->pid);

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
                logger::get_logger()->error("Invalid size for `operation_type::system_preferences_updated`.");
              } else {
                auto p = reinterpret_cast<operation_type_system_preferences_updated_struct*>(&((*buffer)[0]));

                system_preferences_ = p->system_preferences;

                if (device_grabber_) {
                  device_grabber_->async_set_system_preferences(p->system_preferences);
                }

                logger::get_logger()->info("`system_preferences` is updated.");
              }
              break;

            default:
              break;
          }
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

    device_grabber_ = std::make_unique<device_grabber>(weak_grabbable_state_queues_manager_,
                                                       console_user_server_client_);

    device_grabber_->async_set_system_preferences(system_preferences_);
    device_grabber_->async_post_frontmost_application_changed_event(frontmost_application_);
    device_grabber_->async_post_input_source_changed_event(input_source_properties_);

    device_grabber_->async_start(configuration_file_path);

    logger::get_logger()->info("device_grabber is started.");
  }

  void stop_device_grabber(void) {
    if (!device_grabber_) {
      return;
    }

    device_grabber_ = nullptr;

    logger::get_logger()->info("device_grabber is stopped.");
  }

  std::weak_ptr<grabbable_state_queues_manager> weak_grabbable_state_queues_manager_;

  std::unique_ptr<pqrs::local_datagram::server> server_;
  std::shared_ptr<console_user_server_client> console_user_server_client_;
  std::unique_ptr<device_grabber> device_grabber_;

  system_preferences system_preferences_;
  pqrs::osx::frontmost_application_monitor::application frontmost_application_;
  pqrs::osx::input_source::properties input_source_properties_;
};
} // namespace krbn
