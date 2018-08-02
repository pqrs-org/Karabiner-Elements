#pragma once

#include "constants.hpp"
#include "device_grabber.hpp"
#include "grabbable_state_queues_manager.hpp"
#include "local_datagram/server_manager.hpp"
#include "process_monitor.hpp"
#include "session.hpp"
#include "types.hpp"
#include <vector>

namespace krbn {
class receiver final {
public:
  receiver(const receiver&) = delete;

  receiver(void) {
    std::string socket_file_path(constants::get_grabber_socket_file_path());

    unlink(socket_file_path.c_str());

    size_t buffer_size = 32 * 1024;
    std::chrono::milliseconds server_check_interval(3000);
    std::chrono::milliseconds reconnect_interval(1000);

    server_manager_ = std::make_unique<local_datagram::server_manager>(socket_file_path,
                                                                       buffer_size,
                                                                       server_check_interval,
                                                                       reconnect_interval);

    server_manager_->bound.connect([socket_file_path] {
      if (auto uid = session::get_current_console_user_id()) {
        chown(socket_file_path.c_str(), *uid, 0);
      }
      chmod(socket_file_path.c_str(), 0600);

      grabbable_state_queues_manager::get_shared_instance()->clear();
    });

    server_manager_->received.connect([this](auto&& buffer) {
      if (auto type = types::find_operation_type(buffer.data(), buffer.size())) {
        switch (*type) {
          case operation_type::grabbable_state_changed:
            if (buffer.size() != sizeof(operation_type_grabbable_state_changed_struct)) {
              logger::get_logger().error("Invalid size for `operation_type::grabbable_state_changed`.");
            } else {
              auto p = reinterpret_cast<operation_type_grabbable_state_changed_struct*>(buffer.data());

              gcd_utility::dispatch_sync_in_main_queue(^{
                grabbable_state_queues_manager::get_shared_instance()->update_grabbable_state(p->grabbable_state);
              });
            }
            break;

          case operation_type::connect:
            if (buffer.size() != sizeof(operation_type_connect_struct)) {
              logger::get_logger().error("Invalid size for `operation_type::connect`.");
            } else {
              auto p = reinterpret_cast<operation_type_connect_struct*>(buffer.data());

              // Ensure user_core_configuration_file_path is null-terminated string even if corrupted data is sent.
              p->user_core_configuration_file_path[sizeof(p->user_core_configuration_file_path) - 1] = '\0';

              logger::get_logger().info("karabiner_console_user_server is connected (pid:{0})", p->pid);

              gcd_utility::dispatch_sync_in_main_queue(^{
                start_device_grabber(p->user_core_configuration_file_path);

                // monitor the last process
                {
                  std::lock_guard<std::mutex> lock(console_user_server_process_monitor_mutex_);

                  console_user_server_process_monitor_ = nullptr;
                  console_user_server_process_monitor_ = std::make_unique<process_monitor>(p->pid,
                                                                                           std::bind(&receiver::console_user_server_exit_callback, this));
                }
              });
            }
            break;

          case operation_type::system_preferences_updated:
            if (buffer.size() < sizeof(operation_type_system_preferences_updated_struct)) {
              logger::get_logger().error("Invalid size for `operation_type::system_preferences_updated`.");
            } else {
              auto p = reinterpret_cast<operation_type_system_preferences_updated_struct*>(buffer.data());

              gcd_utility::dispatch_sync_in_main_queue(^{
                std::lock_guard<std::mutex> lock(device_grabber_mutex_);

                if (device_grabber_) {
                  device_grabber_->set_system_preferences(p->system_preferences);
                  logger::get_logger().info("system_preferences_updated");
                }
              });
            }
            break;

          case operation_type::frontmost_application_changed:
            if (buffer.size() < sizeof(operation_type_frontmost_application_changed_struct)) {
              logger::get_logger().error("Invalid size for `operation_type::frontmost_application_changed`.");
            } else {
              auto p = reinterpret_cast<operation_type_frontmost_application_changed_struct*>(buffer.data());

              // Ensure bundle_identifier and file_path are null-terminated string even if corrupted data is sent.
              p->bundle_identifier[sizeof(p->bundle_identifier) - 1] = '\0';
              p->file_path[sizeof(p->file_path) - 1] = '\0';

              gcd_utility::dispatch_sync_in_main_queue(^{
                std::lock_guard<std::mutex> lock(device_grabber_mutex_);

                if (device_grabber_) {
                  device_grabber_->post_frontmost_application_changed_event(p->bundle_identifier,
                                                                            p->file_path);
                }
              });
            }
            break;

          case operation_type::input_source_changed:
            if (buffer.size() < sizeof(operation_type_input_source_changed_struct)) {
              logger::get_logger().error("Invalid size for `operation_type::input_source_changed`.");
            } else {
              auto p = reinterpret_cast<operation_type_input_source_changed_struct*>(buffer.data());

              // Ensure bundle_identifier and file_path are null-terminated string even if corrupted data is sent.
              p->language[sizeof(p->language) - 1] = '\0';
              p->input_source_id[sizeof(p->input_source_id) - 1] = '\0';
              p->input_mode_id[sizeof(p->input_mode_id) - 1] = '\0';

              gcd_utility::dispatch_sync_in_main_queue(^{
                std::lock_guard<std::mutex> lock(device_grabber_mutex_);

                if (device_grabber_) {
                  device_grabber_->post_input_source_changed_event({std::string(p->language),
                                                                    std::string(p->input_source_id),
                                                                    std::string(p->input_mode_id)});
                }
              });
            }
            break;

          default:
            break;
        }
      }
    });

    server_manager_->start();

    start_grabbing_if_system_core_configuration_file_exists();

    logger::get_logger().info("receiver is initialized");
  }

  ~receiver(void) {
    server_manager_ = nullptr;

    {
      std::lock_guard<std::mutex> lock(console_user_server_process_monitor_mutex_);
      console_user_server_process_monitor_ = nullptr;
    }

    stop_device_grabber();

    logger::get_logger().info("receiver is terminated");
  }

private:
  void console_user_server_exit_callback(void) {
    stop_device_grabber();

    start_grabbing_if_system_core_configuration_file_exists();
  }

  void start_grabbing_if_system_core_configuration_file_exists(void) {
    auto file_path = constants::get_system_core_configuration_file_path();
    if (filesystem::exists(file_path)) {
      start_device_grabber(file_path);
    }
  }

  void start_device_grabber(const std::string& configuration_file_path) {
    std::lock_guard<std::mutex> lock(device_grabber_mutex_);

    device_grabber_ = nullptr;
    device_grabber_ = std::make_unique<device_grabber>();
    device_grabber_->start(configuration_file_path);
  }

  void stop_device_grabber(void) {
    std::lock_guard<std::mutex> lock(device_grabber_mutex_);

    device_grabber_ = nullptr;
  }

  std::unique_ptr<local_datagram::server_manager> server_manager_;

  std::unique_ptr<process_monitor> console_user_server_process_monitor_;
  std::mutex console_user_server_process_monitor_mutex_;

  std::unique_ptr<device_grabber> device_grabber_;
  std::mutex device_grabber_mutex_;
};
} // namespace krbn
