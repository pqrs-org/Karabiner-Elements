#pragma once

#include "apple_notification_center.hpp"
#include "constants.hpp"
#include "device_observer.hpp"
#include "grabber_client.hpp"
#include "version_monitor.hpp"

namespace krbn {
class connection_manager final {
public:
  connection_manager(const connection_manager&) = delete;

  connection_manager(version_monitor& version_monitor) : version_monitor_(version_monitor) {
    apple_notification_center::observe_distributed_notification(this,
                                                                static_grabber_is_launched_callback,
                                                                constants::get_distributed_notification_grabber_is_launched());

    timer_ = std::make_unique<gcd_utility::main_queue_timer>(
        dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC),
        1 * NSEC_PER_SEC,
        0,
        ^{
          auto actual_uid = session::get_current_console_user_id();
          auto uid = actual_uid;
          if (!uid) {
            // Ensure last_uid_ != uid at the first timer invocation.
            uid = 0;
          }

          if (last_uid_ != uid) {
            if (actual_uid) {
              logger::get_logger().info("current_console_user_id: {0}", *actual_uid);
            } else {
              logger::get_logger().info("current_console_user_id: none");
            }

            version_monitor_.manual_check();

            try {
              release();

              grabber_client_ = std::make_unique<grabber_client>();
              device_observer_ = std::make_unique<device_observer>(*grabber_client_);

              last_uid_ = uid;
            } catch (std::exception& ex) {
              logger::get_logger().warn(ex.what());
            }
          }
        });
  }

  ~connection_manager(void) {
    timer_ = nullptr;

    gcd_utility::dispatch_sync_in_main_queue(^{
      release();
    });

    apple_notification_center::unobserve_distributed_notification(this,
                                                                  constants::get_distributed_notification_grabber_is_launched());
  }

private:
  void release(void) {
    last_uid_ = boost::none;
    device_observer_ = nullptr;
    grabber_client_ = nullptr;
  }

  static void static_grabber_is_launched_callback(CFNotificationCenterRef center,
                                                  void* observer,
                                                  CFStringRef notification_name,
                                                  const void* observed_object,
                                                  CFDictionaryRef user_info) {
    auto self = static_cast<connection_manager*>(observer);
    self->grabber_is_launched_callback();
  }

  void grabber_is_launched_callback(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      logger::get_logger().info("connection_manager::grabber_is_launched_callback");
      release();
    });
  }

  version_monitor& version_monitor_;

  std::unique_ptr<gcd_utility::main_queue_timer> timer_;

  boost::optional<uid_t> last_uid_;

  std::unique_ptr<grabber_client> grabber_client_;
  std::unique_ptr<device_observer> device_observer_;
};
} // namespace krbn
