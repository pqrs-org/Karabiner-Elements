#pragma once

#include "console_user_server_client.hpp"
#include "constants.hpp"
#include "device_grabber.hpp"
#include "gcd_utility.hpp"
#include "logger.hpp"
#include "notification_center.hpp"
#include "receiver.hpp"
#include "session.hpp"
#include "version_monitor.hpp"
#include <sys/stat.h>
#include <unistd.h>

namespace krbn {
class connection_manager final {
public:
  connection_manager(const connection_manager&) = delete;

  connection_manager(version_monitor& version_monitor,
                     device_grabber& device_grabber) : version_monitor_(version_monitor),
                                                       device_grabber_(device_grabber),
                                                       timer_(nullptr) {
    timer_ = std::make_unique<gcd_utility::main_queue_timer>(
        dispatch_time(DISPATCH_TIME_NOW, 1.0 * NSEC_PER_SEC),
        1.0 * NSEC_PER_SEC,
        0,
        ^{
          auto actual_uid = session::get_current_console_user_id();
          auto uid = actual_uid;
          if (!uid) {
            // Ensure last_uid_ != uid at the first timer invocation.
            uid = 0;
          }

          if (last_uid_ != uid) {
            last_uid_ = uid;

            if (actual_uid) {
              logger::get_logger().info("current_console_user_id: {0}", *actual_uid);
            } else {
              logger::get_logger().info("current_console_user_id: none");
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
          }
        });
  }

  ~connection_manager(void) {
    timer_ = nullptr;
    receiver_ = nullptr;
  }

private:
  version_monitor& version_monitor_;
  device_grabber& device_grabber_;

  std::unique_ptr<gcd_utility::main_queue_timer> timer_;

  boost::optional<uid_t> last_uid_;

  std::unique_ptr<receiver> receiver_;
};
} // namespace krbn
