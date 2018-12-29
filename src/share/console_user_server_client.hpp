#pragma once

#include "constants.hpp"
#include "local_datagram/client_manager.hpp"
#include "logger.hpp"
#include "monitor/console_user_id_monitor.hpp"
#include "types.hpp"
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <sstream>
#include <unistd.h>
#include <vector>

namespace krbn {
class console_user_server_client final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  // Note: These signals are fired on local_datagram::client's thread.

  nod::signal<void(void)> connected;
  nod::signal<void(const boost::system::error_code&)> connect_failed;
  nod::signal<void(void)> closed;

  // Methods

  console_user_server_client(const console_user_server_client&) = delete;

  console_user_server_client(void) : dispatcher_client() {
    console_user_id_monitor_ = std::make_unique<console_user_id_monitor>();

    console_user_id_monitor_->console_user_id_changed.connect([this](std::optional<uid_t> uid) {
      if (uid) {
        client_manager_ = nullptr;

        auto socket_file_path = make_console_user_server_socket_file_path(*uid);
        std::chrono::milliseconds server_check_interval(3000);
        std::chrono::milliseconds reconnect_interval(1000);

        client_manager_ = std::make_unique<local_datagram::client_manager>(socket_file_path,
                                                                           server_check_interval,
                                                                           reconnect_interval);

        client_manager_->connected.connect([this, uid] {
          logger::get_logger()->info("console_user_server_client is connected. (uid:{0})", *uid);

          enqueue_to_dispatcher([this] {
            connected();
          });
        });

        client_manager_->connect_failed.connect([this](auto&& error_code) {
          enqueue_to_dispatcher([this, error_code] {
            connect_failed(error_code);
          });
        });

        client_manager_->closed.connect([this, uid] {
          logger::get_logger()->info("console_user_server_client is closed. (uid:{0})", *uid);

          enqueue_to_dispatcher([this] {
            closed();
          });
        });

        client_manager_->async_start();
      }
    });
  }

  virtual ~console_user_server_client(void) {
    detach_from_dispatcher([this] {
      console_user_id_monitor_ = nullptr;

      client_manager_ = nullptr;
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      console_user_id_monitor_->async_start();
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      console_user_id_monitor_->async_stop();

      client_manager_ = nullptr;
    });
  }

  void async_shell_command_execution(const std::string& shell_command) const {
    enqueue_to_dispatcher([this, shell_command] {
      operation_type_shell_command_execution_struct s;

      if (shell_command.length() >= sizeof(s.shell_command)) {
        logger::get_logger()->error("shell_command is too long: {0}", shell_command);
        return;
      }

      strlcpy(s.shell_command,
              shell_command.c_str(),
              sizeof(s.shell_command));

      async_send(reinterpret_cast<const uint8_t*>(&s), sizeof(s));
    });
  }

  void async_select_input_source(const input_source_selector& input_source_selector,
                                 absolute_time_point time_stamp) {
    enqueue_to_dispatcher([this, input_source_selector, time_stamp] {
      operation_type_select_input_source_struct s;
      s.time_stamp = time_stamp;

      if (auto& v = input_source_selector.get_language_string()) {
        if (v->length() >= sizeof(s.language)) {
          logger::get_logger()->error("language is too long: {0}", *v);
          return;
        }

        strlcpy(s.language,
                v->c_str(),
                sizeof(s.language));
      }

      if (auto& v = input_source_selector.get_input_source_id_string()) {
        if (v->length() >= sizeof(s.input_source_id)) {
          logger::get_logger()->error("input_source_id is too long: {0}", *v);
          return;
        }

        strlcpy(s.input_source_id,
                v->c_str(),
                sizeof(s.input_source_id));
      }

      if (auto& v = input_source_selector.get_input_mode_id_string()) {
        if (v->length() >= sizeof(s.input_mode_id)) {
          logger::get_logger()->error("input_mode_id is too long: {0}", *v);
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

  std::unique_ptr<console_user_id_monitor> console_user_id_monitor_;
  std::unique_ptr<local_datagram::client_manager> client_manager_;
};
} // namespace krbn
