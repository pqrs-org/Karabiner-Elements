#pragma once

#include "console_user_id_monitor.hpp"
#include "console_user_server_client.hpp"
#include "constants.hpp"
#include "device_grabber.hpp"
#include "gcd_utility.hpp"
#include "logger.hpp"
#include "receiver.hpp"
#include "session.hpp"
#include "version_monitor.hpp"

namespace krbn {
class connection_manager final {
public:
  connection_manager(const connection_manager&) = delete;

  connection_manager(version_monitor& version_monitor,
                     device_grabber& device_grabber) : version_monitor_(version_monitor),
                                                       device_grabber_(device_grabber) {
    console_user_id_monitor_.console_user_id_changed.connect([this](boost::optional<uid_t> uid) {
      if (uid) {
        logger::get_logger().info("current_console_user_id: {0}", *uid);
      } else {
        logger::get_logger().info("current_console_user_id: none");
        uid = 0;
      }

      version_monitor_.manual_check();

      // Prepare console_user_server_socket_directory
      {
        auto socket_file_path = console_user_server_client::make_console_user_server_socket_directory(*uid);
        mkdir(socket_file_path.c_str(), 0700);
        chown(socket_file_path.c_str(), *uid, 0);
        chmod(socket_file_path.c_str(), 0700);
      }

      receiver_ = nullptr;
      receiver_ = std::make_unique<receiver>(device_grabber_);
    });

    console_user_id_monitor_.start();
  }

  ~connection_manager(void) {
    receiver_ = nullptr;
    console_user_id_monitor_.stop();
  }

private:
  version_monitor& version_monitor_;
  device_grabber& device_grabber_;

  console_user_id_monitor console_user_id_monitor_;
  std::unique_ptr<receiver> receiver_;
};
} // namespace krbn
