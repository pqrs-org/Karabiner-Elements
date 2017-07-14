#pragma once

#include "constants.hpp"
#include "device_grabber.hpp"
#include "local_datagram_server.hpp"
#include "process_monitor.hpp"
#include "session.hpp"
#include "types.hpp"
#include <vector>

namespace krbn {
class receiver final {
public:
  receiver(const receiver&) = delete;

  receiver(device_grabber& device_grabber) : device_grabber_(device_grabber),
                                             exit_loop_(false) {
    const size_t buffer_length = 32 * 1024;
    buffer_.resize(buffer_length);

    const char* path = constants::get_grabber_socket_file_path();
    unlink(path);
    server_ = std::make_unique<local_datagram_server>(path);

    if (auto uid = session::get_current_console_user_id()) {
      chown(path, *uid, 0);
    }
    chmod(path, 0600);

    start_grabbing_if_system_core_configuration_file_exists();

    exit_loop_ = false;
    thread_ = std::thread([this] { this->worker(); });
  }

  ~receiver(void) {
    unlink(constants::get_grabber_socket_file_path());

    exit_loop_ = true;
    if (thread_.joinable()) {
      thread_.join();
    }

    server_ = nullptr;
    console_user_server_process_monitor_ = nullptr;
    device_grabber_.stop_grabbing();
    device_grabber_.unset_profile();
  }

private:
  void worker(void) {
    if (!server_) {
      return;
    }

    while (!exit_loop_) {
      boost::system::error_code ec;
      std::size_t n = server_->receive(boost::asio::buffer(buffer_), boost::posix_time::seconds(1), ec);

      if (!ec && n > 0) {
        switch (operation_type(buffer_[0])) {
          case operation_type::connect:
            if (n != sizeof(operation_type_connect_struct)) {
              logger::get_logger().error("invalid size for operation_type::connect");
            } else {
              auto p = reinterpret_cast<operation_type_connect_struct*>(&(buffer_[0]));

              // Ensure user_core_configuration_file_path is null-terminated string even if corrupted data is sent.
              p->user_core_configuration_file_path[sizeof(p->user_core_configuration_file_path) - 1] = '\0';

              logger::get_logger().info("karabiner_console_user_server is connected (pid:{0})", p->pid);

              device_grabber_.start_grabbing(p->user_core_configuration_file_path);

              // monitor the last process
              console_user_server_process_monitor_ = nullptr;
              console_user_server_process_monitor_ = std::make_unique<process_monitor>(p->pid,
                                                                                       std::bind(&receiver::console_user_server_exit_callback, this));
            }
            break;

          case operation_type::system_preferences_values_updated:
            if (n < sizeof(operation_type_system_preferences_values_updated_struct)) {
              logger::get_logger().error("invalid size for operation_type::system_preferences_values_updated ({0})", n);
            } else {
              auto p = reinterpret_cast<operation_type_system_preferences_values_updated_struct*>(&(buffer_[0]));
              device_grabber_.set_system_preferences_values(p->values);
              logger::get_logger().info("system_preferences_values_updated");
            }
            break;

          case operation_type::frontmost_application_changed:
            if (n < sizeof(operation_type_frontmost_application_changed_struct)) {
              logger::get_logger().error("invalid size for operation_type::frontmost_application_changed ({0})", n);
            } else {
              auto p = reinterpret_cast<operation_type_frontmost_application_changed_struct*>(&(buffer_[0]));

              // Ensure bundle_identifier and file_path are null-terminated string even if corrupted data is sent.
              p->bundle_identifier[sizeof(p->bundle_identifier) - 1] = '\0';
              p->file_path[sizeof(p->file_path) - 1] = '\0';

              device_grabber_.post_frontmost_application_changed_event(p->bundle_identifier,
                                                                       p->file_path);
            }
            break;

          default:
            break;
        }
      }
    }
  }

  void console_user_server_exit_callback(void) {
    device_grabber_.post_frontmost_application_changed_event("", "");

    device_grabber_.stop_grabbing();

    start_grabbing_if_system_core_configuration_file_exists();
  }

  void start_grabbing_if_system_core_configuration_file_exists(void) {
    auto file_path = constants::get_system_core_configuration_file_path();
    if (filesystem::exists(file_path)) {
      device_grabber_.start_grabbing(file_path);
    }
  }

  device_grabber& device_grabber_;

  std::vector<uint8_t> buffer_;
  std::unique_ptr<local_datagram_server> server_;
  std::thread thread_;
  std::atomic<bool> exit_loop_;

  std::unique_ptr<process_monitor> console_user_server_process_monitor_;
};
} // namespace krbn
