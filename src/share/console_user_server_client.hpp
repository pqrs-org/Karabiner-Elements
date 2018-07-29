#pragma once

#include "boost_defs.hpp"

#include "console_user_id_monitor.hpp"
#include "constants.hpp"
#include "filesystem.hpp"
#include "gcd_utility.hpp"
#include "local_datagram/client.hpp"
#include "logger.hpp"
#include "session.hpp"
#include "shared_instance_provider.hpp"
#include "types.hpp"
#include <boost/signals2.hpp>
#include <sstream>
#include <unistd.h>
#include <vector>

namespace krbn {
class console_user_server_client final : public shared_instance_provider<console_user_server_client> {
public:
  // Signals

  // Note: These signals are fired on local_datagram::client's thread.

  boost::signals2::signal<void(void)> connected;
  boost::signals2::signal<void(const boost::system::error_code&)> connect_failed;
  boost::signals2::signal<void(void)> closed;

  // Methods

  console_user_server_client(const console_user_server_client&) = delete;

  console_user_server_client(void) {
    console_user_id_monitor_.console_user_id_changed.connect([this](boost::optional<uid_t> uid) {
      if (uid) {
        connect(*uid);
      }
    });
  }

  void start(void) {
    console_user_id_monitor_.start();
  }

  void shell_command_execution(const std::string& shell_command) {
    operation_type_shell_command_execution_struct s;

    if (shell_command.length() >= sizeof(s.shell_command)) {
      logger::get_logger().error("shell_command is too long: {0}", shell_command);
      return;
    }

    strlcpy(s.shell_command,
            shell_command.c_str(),
            sizeof(s.shell_command));

    dispatch_async(dispatch_get_main_queue(), ^{
      if (client_) {
        client_->async_send(reinterpret_cast<const uint8_t*>(&s), sizeof(s));
      }
    });
  }

  void select_input_source(const input_source_selector& input_source_selector, uint64_t time_stamp) {
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

    dispatch_async(dispatch_get_main_queue(), ^{
      if (client_) {
        client_->async_send(reinterpret_cast<const uint8_t*>(&s), sizeof(s));
      }
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
  void connect(uid_t uid) {
    auto socket_file_path = make_console_user_server_socket_file_path(uid);

    connect_retry_timer_ = nullptr;
    client_ = nullptr;

    client_ = std::make_unique<local_datagram::client>();

    client_->connected.connect([this, uid](void) {
      logger::get_logger().info("console_user_server_client is connected. (uid:{0})", uid);

      connected();
    });

    client_->connect_failed.connect([this, uid](auto&& error_code) {
      connect_failed(error_code);

      start_connect_retry_timer(uid);
    });

    client_->closed.connect([this, uid](void) {
      logger::get_logger().info("console_user_server_client is closed. (uid:{0})", uid);

      closed();

      start_connect_retry_timer(uid);
    });

    client_->connect(socket_file_path,
                     std::chrono::milliseconds(3000));
  }

  void start_connect_retry_timer(uid_t uid) {
    connect_retry_timer_ = std::make_unique<gcd_utility::main_queue_after_timer>(
        dispatch_time(DISPATCH_TIME_NOW, 3.0 * NSEC_PER_SEC),
        false,
        ^{
          connect(uid);
        });
  }

  console_user_id_monitor console_user_id_monitor_;
  std::unique_ptr<local_datagram::client> client_;
  std::unique_ptr<gcd_utility::main_queue_after_timer> connect_retry_timer_;
};
} // namespace krbn
