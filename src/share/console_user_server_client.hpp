#pragma once

#include "constants.hpp"
#include "local_datagram/client_manager.hpp"
#include "logger.hpp"
#include "monitor/console_user_id_monitor.hpp"
#include "types.hpp"
#include <sstream>
#include <unistd.h>
#include <vector>

namespace krbn {
class console_user_server_client final {
public:
  // Signals

  // Note: These signals are fired on local_datagram::client's thread.

  boost::signals2::signal<void(void)> connected;
  boost::signals2::signal<void(const boost::system::error_code&)> connect_failed;
  boost::signals2::signal<void(void)> closed;

  // Methods

  console_user_server_client(const console_user_server_client&) = delete;

  console_user_server_client(void) {
    queue_ = std::make_unique<thread_utility::queue>();

    console_user_id_monitor_ = std::make_unique<console_user_id_monitor>();

    console_user_id_monitor_->console_user_id_changed.connect([this](boost::optional<uid_t> uid) {
      queue_->push_back([this, uid] {
        if (uid) {
          client_manager_ = nullptr;

          auto socket_file_path = make_console_user_server_socket_file_path(*uid);
          std::chrono::milliseconds server_check_interval(3000);
          std::chrono::milliseconds reconnect_interval(1000);

          client_manager_ = std::make_unique<local_datagram::client_manager>(socket_file_path,
                                                                             server_check_interval,
                                                                             reconnect_interval);

          client_manager_->connected.connect([this, uid] {
            logger::get_logger().info("console_user_server_client is connected. (uid:{0})", *uid);

            connected();
          });

          client_manager_->connect_failed.connect([this](auto&& error_code) {
            connect_failed(error_code);
          });

          client_manager_->closed.connect([this, uid] {
            logger::get_logger().info("console_user_server_client is closed. (uid:{0})", *uid);

            closed();
          });

          client_manager_->start();
        }
      });
    });
  }

  ~console_user_server_client(void) {
    queue_->push_back([this] {
      console_user_id_monitor_ = nullptr;

      client_manager_ = nullptr;
    });

    queue_->terminate();
    queue_ = nullptr;
  }

  void async_start(void) {
    queue_->push_back([this] {
      console_user_id_monitor_->async_start();
    });
  }

  void async_stop(void) {
    queue_->push_back([this] {
      console_user_id_monitor_->async_stop();

      client_manager_ = nullptr;
    });
  }

  void async_shell_command_execution(const std::string& shell_command) const {
    queue_->push_back([this, shell_command] {
      operation_type_shell_command_execution_struct s;

      if (shell_command.length() >= sizeof(s.shell_command)) {
        logger::get_logger().error("shell_command is too long: {0}", shell_command);
        return;
      }

      strlcpy(s.shell_command,
              shell_command.c_str(),
              sizeof(s.shell_command));

      async_send(reinterpret_cast<const uint8_t*>(&s), sizeof(s));
    });
  }

  void async_select_input_source(const input_source_selector& input_source_selector, uint64_t time_stamp) {
    queue_->push_back([this, input_source_selector, time_stamp] {
      operation_type_select_input_source_struct s;
      s.time_stamp = time_stamp;

      if (auto& v = input_source_selector.get_language_string()) {
        if (v->length() >= sizeof(s.language)) {
          logger::get_logger().error("language is too long: {0}", *v);
          return;
        }

        strlcpy(s.language,
                v->c_str(),
                sizeof(s.language));
      }

      if (auto& v = input_source_selector.get_input_source_id_string()) {
        if (v->length() >= sizeof(s.input_source_id)) {
          logger::get_logger().error("input_source_id is too long: {0}", *v);
          return;
        }

        strlcpy(s.input_source_id,
                v->c_str(),
                sizeof(s.input_source_id));
      }

      if (auto& v = input_source_selector.get_input_mode_id_string()) {
        if (v->length() >= sizeof(s.input_mode_id)) {
          logger::get_logger().error("input_mode_id is too long: {0}", *v);
          return;
        }

        strlcpy(s.input_mode_id,
                v->c_str(),
                sizeof(s.input_mode_id));
      }

      async_send(reinterpret_cast<const uint8_t*>(&s), sizeof(s));
    });
  }

  static std::string make_console_user_server_socket_directory(uid_t uid) {
    std::stringstream ss;
    ss << constants::get_console_user_server_socket_directory() << "/" << uid;
    return ss.str();
  }

  static std::string make_console_user_server_socket_file_path(uid_t uid) {
    return make_console_user_server_socket_directory(uid) + "/receiver";
  }

private:
  void async_send(const uint8_t* _Nonnull p, size_t length) const {
    if (client_manager_) {
      if (auto client = client_manager_->get_client()) {
        client->async_send(p, length);
      }
    }
  }

  std::unique_ptr<thread_utility::queue> queue_;
  std::unique_ptr<console_user_id_monitor> console_user_id_monitor_;
  std::unique_ptr<local_datagram::client_manager> client_manager_;
};
} // namespace krbn
