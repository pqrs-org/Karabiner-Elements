#pragma once

#include "gcd_utility.hpp"
#include "logger.hpp"
#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

namespace krbn {
class iopm_client final {
public:
  typedef std::function<void(uint32_t message_type)> system_power_event_callback;

  iopm_client(const iopm_client&) = delete;

  iopm_client(const system_power_event_callback& system_power_event_callback) : system_power_event_callback_(system_power_event_callback),
                                                                                notification_port_(nullptr),
                                                                                notifier_(IO_OBJECT_NULL),
                                                                                connect_(IO_OBJECT_NULL) {
    auto connect = IORegisterForSystemPower(this, &notification_port_, static_callback, &notifier_);
    if (connect == MACH_PORT_NULL) {
      logger::get_logger().error("IORegisterForSystemPower error @ {0}", __PRETTY_FUNCTION__);

    } else {
      if (auto run_loop_source = IONotificationPortGetRunLoopSource(notification_port_)) {
        CFRunLoopAddSource(CFRunLoopGetMain(), run_loop_source, kCFRunLoopCommonModes);
      }
    }
  }

  ~iopm_client(void) {
    // Release notifier_ in main thread to avoid callback invocations after object has been destroyed.
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (notifier_) {
        auto kr = IODeregisterForSystemPower(&notifier_);
        if (kr != kIOReturnSuccess) {
          logger::get_logger().error("IODeregisterForSystemPower error: {1} @ {0}", __PRETTY_FUNCTION__, kr);
        }
      }
      if (notification_port_) {
        IONotificationPortDestroy(notification_port_);
      }
      if (connect_) {
        auto kr = IOServiceClose(connect_);
        if (kr != kIOReturnSuccess) {
          logger::get_logger().error("IOServiceClose error: {1} @ {0}", __PRETTY_FUNCTION__, kr);
        }
        connect_ = IO_OBJECT_NULL;
      }
    });
  }

private:
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
            logger::get_logger().error("IOAllowPowerChange error: {1} @ {0}", __PRETTY_FUNCTION__, kr);
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
            logger::get_logger().error("IOAllowPowerChange error: {1} @ {0}", __PRETTY_FUNCTION__, kr);
          }
        }
        break;

      case kIOMessageSystemWillNotSleep:
        logger::get_logger().info("iopm_client::callback kIOMessageSystemWillNotSleep");
        break;
    }

    if (system_power_event_callback_) {
      system_power_event_callback_(message_type);
    }
  }

  system_power_event_callback system_power_event_callback_;

  IONotificationPortRef notification_port_;
  io_object_t notifier_;
  io_connect_t connect_;
};
} // namespace krbn
