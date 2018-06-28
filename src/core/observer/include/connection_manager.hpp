#pragma once

#include "console_user_id_monitor.hpp"
#include "constants.hpp"
#include "device_observer.hpp"
#include "gcd_utility.hpp"
#include "grabber_client.hpp"
#include "version_monitor.hpp"

namespace krbn {
class connection_manager final {
public:
  connection_manager(const connection_manager&) = delete;

  connection_manager(version_monitor& version_monitor) : version_monitor_(version_monitor) {
    console_user_id_monitor_.console_user_id_changed.connect([this](boost::optional<uid_t> uid) {
      if (uid) {
        logger::get_logger().info("current_console_user_id: {0}", *uid);
      } else {
        logger::get_logger().info("current_console_user_id: none");
        uid = 0;
      }

      version_monitor_.manual_check();
      release();

      timer_ = std::make_unique<gcd_utility::fire_while_false_timer>(3 * NSEC_PER_SEC,
                                                                     ^{
                                                                       return setup();
                                                                     });
    });
    console_user_id_monitor_.start();
  }

  ~connection_manager(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      release();
    });

    console_user_id_monitor_.stop();
  }

private:
  void release(void) {
    device_observer_ = nullptr;
    grabber_client_ = nullptr;
    timer_ = nullptr;
  }

  bool setup(void) {
    try {
      grabber_client_ = std::make_unique<grabber_client>();
      device_observer_ = std::make_unique<device_observer>(*grabber_client_);

      return true;
    } catch (std::exception& ex) {
      logger::get_logger().warn(ex.what());
    }

    return false;
  }

  version_monitor& version_monitor_;

  console_user_id_monitor console_user_id_monitor_;
  std::unique_ptr<gcd_utility::fire_while_false_timer> timer_;

  std::unique_ptr<grabber_client> grabber_client_;
  std::unique_ptr<device_observer> device_observer_;
};
} // namespace krbn
