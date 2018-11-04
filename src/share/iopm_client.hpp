#pragma once

#include "boost_defs.hpp"

#include "dispatcher.hpp"
#include "iokit_utility.hpp"
#include "logger.hpp"
#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <boost/signals2.hpp>
#include <pqrs/cf_run_loop_thread.hpp>

namespace krbn {
class iopm_client final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  boost::signals2::signal<void(uint32_t)> system_power_event_arrived;

  // Methods

  iopm_client(const iopm_client&) = delete;

  iopm_client(void) : dispatcher_client(),
                      notification_port_(nullptr),
                      notifier_(IO_OBJECT_NULL),
                      connect_(IO_OBJECT_NULL) {
    cf_run_loop_thread_ = std::make_unique<pqrs::cf_run_loop_thread>();
  }

  virtual ~iopm_client(void) {
    detach_from_dispatcher([this] {
      stop();
    });

    cf_run_loop_thread_->terminate();
    cf_run_loop_thread_ = nullptr;
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      start();
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

private:
  // This method is executed in the dispatcher thread.
  void start(void) {
    if (notification_port_) {
      return;
    }

    auto connect = IORegisterForSystemPower(this,
                                            &notification_port_,
                                            static_callback,
                                            &notifier_);
    if (connect == MACH_PORT_NULL) {
      logger::get_logger().error("IORegisterForSystemPower is failed.");
      return;
    }

    if (auto run_loop_source = IONotificationPortGetRunLoopSource(notification_port_)) {
      CFRunLoopAddSource(cf_run_loop_thread_->get_run_loop(),
                         run_loop_source,
                         kCFRunLoopCommonModes);
    }

    logger::get_logger().error("iopm_client is started.");
  }

  // This method is executed in the dispatcher thread.
  void stop(void) {
    if (notifier_) {
      auto kr = IODeregisterForSystemPower(&notifier_);
      if (kr != kIOReturnSuccess) {
        logger::get_logger().error("IODeregisterForSystemPower is failed: {0}",
                                   iokit_utility::get_error_name(kr));
      }
      notifier_ = IO_OBJECT_NULL;
    }

    if (notification_port_) {
      IONotificationPortDestroy(notification_port_);
      notification_port_ = nullptr;
    }

    if (connect_) {
      auto kr = IOServiceClose(connect_);
      if (kr != kIOReturnSuccess) {
        logger::get_logger().error("IOServiceClose is failed: {0}",
                                   iokit_utility::get_error_name(kr));
      }
      connect_ = IO_OBJECT_NULL;
    }

    logger::get_logger().error("iopm_client is stopped.");
  }

  static void static_callback(void* refcon, io_service_t service, uint32_t message_type, void* message_argument) {
    iopm_client* self = static_cast<iopm_client*>(refcon);
    if (self) {
      self->callback(service, message_type, message_argument);
    }
  }

  void callback(io_service_t service, uint32_t message_type, void* message_argument) {
    switch (message_type) {
      case kIOMessageSystemWillSleep:
        logger::get_logger().info("iopm_client::callback kIOMessageSystemWillSleep");
        if (connect_) {
          auto kr = IOAllowPowerChange(connect_, reinterpret_cast<intptr_t>(message_argument));
          if (kr != kIOReturnSuccess) {
            logger::get_logger().error("IOAllowPowerChange is failed: {0}",
                                       iokit_utility::get_error_name(kr));
          }
        }
        break;

      case kIOMessageSystemWillPowerOn:
        logger::get_logger().info("iopm_client::callback kIOMessageSystemWillPowerOn");
        break;

      case kIOMessageSystemHasPoweredOn:
        logger::get_logger().info("iopm_client::callback kIOMessageSystemHasPoweredOn");
        break;

      case kIOMessageCanSystemSleep:
        logger::get_logger().info("iopm_client::callback kIOMessageCanSystemSleep");
        if (connect_) {
          auto kr = IOAllowPowerChange(connect_, reinterpret_cast<intptr_t>(message_argument));
          if (kr != kIOReturnSuccess) {
            logger::get_logger().error("IOAllowPowerChange is failed: {0}",
                                       iokit_utility::get_error_name(kr));
          }
        }
        break;

      case kIOMessageSystemWillNotSleep:
        logger::get_logger().info("iopm_client::callback kIOMessageSystemWillNotSleep");
        break;
    }

    enqueue_to_dispatcher([this, message_type] {
      system_power_event_arrived(message_type);
    });
  }

  std::unique_ptr<pqrs::cf_run_loop_thread> cf_run_loop_thread_;
  IONotificationPortRef notification_port_;
  io_object_t notifier_;
  io_connect_t connect_;
};
} // namespace krbn
