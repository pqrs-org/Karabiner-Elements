#pragma once

// `krbn::components_manager` can be used safely in a multi-threaded environment.

#include "console_user_server_client.hpp"
#include "constants.hpp"
#include "dispatcher.hpp"
#include "logger.hpp"
#include "monitor/console_user_id_monitor.hpp"
#include "monitor/version_monitor.hpp"
#include "monitor/version_monitor_utility.hpp"
#include "receiver.hpp"
#include "session.hpp"

namespace krbn {
class components_manager final {
public:
  components_manager(const components_manager&) = delete;

  components_manager(void) : object_id_(dispatcher::make_new_object_id()) {
    dispatcher_ = std::make_shared<dispatcher::dispatcher>();
    dispatcher_->attach(object_id_);

    version_monitor_ = version_monitor_utility::make_version_monitor_stops_main_run_loop_when_version_changed();

    console_user_id_monitor_ = std::make_unique<console_user_id_monitor>();

    console_user_id_monitor_->console_user_id_changed.connect([this](auto&& uid) {
      dispatcher_->enqueue(
          object_id_,
          [this, uid] {
            uid_t console_user_server_socket_uid = 0;

            if (uid) {
              logger::get_logger().info("current_console_user_id: {0}", *uid);
              console_user_server_socket_uid = *uid;
            } else {
              logger::get_logger().info("current_console_user_id: none");
            }

            if (version_monitor_) {
              version_monitor_->async_manual_check();
            }

            // Prepare console_user_server_socket_directory
            {
              auto socket_file_path = console_user_server_client::make_console_user_server_socket_directory(console_user_server_socket_uid);
              mkdir(socket_file_path.c_str(), 0700);
              chown(socket_file_path.c_str(), console_user_server_socket_uid, 0);
              chmod(socket_file_path.c_str(), 0700);
            }

            receiver_ = nullptr;
            receiver_ = std::make_unique<receiver>(dispatcher_);
          });
    });

    console_user_id_monitor_->async_start();
  }

  ~components_manager(void) {
    dispatcher_->detach(object_id_, [this] {
      console_user_id_monitor_ = nullptr;
      receiver_ = nullptr;
      version_monitor_ = nullptr;
    });

    dispatcher_->terminate();
    dispatcher_ = nullptr;
  }

private:
  std::shared_ptr<dispatcher::dispatcher> dispatcher_;
  dispatcher::object_id object_id_;
  std::shared_ptr<version_monitor> version_monitor_;
  std::unique_ptr<console_user_id_monitor> console_user_id_monitor_;
  std::unique_ptr<receiver> receiver_;
};
} // namespace krbn
