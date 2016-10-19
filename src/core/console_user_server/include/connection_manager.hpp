#pragma once

#include "configuration_manager.hpp"
#include "constants.hpp"
#include "gcd_utility.hpp"
#include "logger.hpp"
#include "notification_center.hpp"
#include "session.hpp"
#include "system_preferences_monitor.hpp"
#include <thread>

class connection_manager final {
public:
  connection_manager(const connection_manager&) = delete;

  connection_manager(void) : timer_(0) {
    notification_center::observe_distributed_notification(this,
                                                          static_grabber_is_launched_callback,
                                                          constants::get_distributed_notification_grabber_is_launched());

    timer_ = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue_.get());
    if (!timer_) {
      logger::get_logger().error("dispatch_source_create error @ {0}", __PRETTY_FUNCTION__);
    } else {
      dispatch_source_set_timer(timer_, dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC), 1 * NSEC_PER_SEC, 0);
      dispatch_source_set_event_handler(timer_, ^{
        if (auto is_active = session::is_active()) {
          if (*is_active) {
            try {
              std::lock_guard<std::mutex> guard(mutex_);

              if (!grabber_client_) {
                grabber_client_ = std::make_unique<grabber_client>();
                grabber_client_->connect(krbn::connect_from::console_user_server);
                logger::get_logger().info("grabber_client_ is connected");
              }

              if (!system_preferences_monitor_) {
                system_preferences_monitor_ = std::make_unique<system_preferences_monitor>(
                    logger::get_logger(),
                    std::bind(&connection_manager::system_preferences_values_updated_callback, this, std::placeholders::_1));
              }

              if (!configuration_manager_) {
                configuration_manager_ = std::make_unique<configuration_manager>(logger::get_logger(),
                                                                                 *grabber_client_);
              }

              return;
            } catch (std::exception& ex) {
              logger::get_logger().warn(ex.what());
            }
          }
        }

        release();
      });
      dispatch_resume(timer_);
    }
  }

  ~connection_manager(void) {
    if (timer_) {
      dispatch_source_cancel(timer_);
      dispatch_release(timer_);
      timer_ = 0;
    }

    release();
  }

private:
  void release(void) {
    std::lock_guard<std::mutex> guard(mutex_);

    configuration_manager_ = nullptr;
    system_preferences_monitor_ = nullptr;
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
    logger::get_logger().info("connection_manager::grabber_is_launched_callback");
    release();
  }

  void system_preferences_values_updated_callback(const system_preferences::values& values) {
    std::lock_guard<std::mutex> guard(mutex_);

    if (grabber_client_) {
      grabber_client_->system_preferences_values_updated(values);
    }
  }

  gcd_utility::scoped_queue queue_;
  dispatch_source_t timer_;

  std::unique_ptr<configuration_manager> configuration_manager_;
  std::unique_ptr<grabber_client> grabber_client_;
  std::unique_ptr<system_preferences_monitor> system_preferences_monitor_;

  std::mutex mutex_;
};
