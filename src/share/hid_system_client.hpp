#pragma once

#include "iokit_utility.hpp"
#include "logger.hpp"
#include "monitor/service_monitor.hpp"
#include "types.hpp"

namespace krbn {
class hid_system_client final : pqrs::dispatcher::extra::dispatcher_client {
public:
  hid_system_client(const hid_system_client&) = delete;

  // Note:
  // OS X shares IOHIDSystem among all input devices even the serial_number of IOHIDSystem is same with the one of the input device.
  //
  // Example:
  //   The matched_callback always contains only one IOHIDSystem even if the following devices are connected.
  //     * Apple Internal Keyboard / Track
  //     * HHKB-BT
  //     * org.pqrs.driver.VirtualHIDKeyboard
  //
  //   The IOHIDSystem object's serial_number is one of the connected devices.
  //
  //   But the IOHIDSystem object is shared by all input devices.
  //   Thus, the IOHIDGetModifierLockState returns true if caps lock is on in one device.

  hid_system_client(void) : dispatcher_client(),
                            service_(IO_OBJECT_NULL),
                            connect_(IO_OBJECT_NULL) {
    if (auto matching_dictionary = IOServiceNameMatching(kIOHIDSystemClass)) {
      service_monitor_ = std::make_unique<monitor::service_monitor::monitor>(matching_dictionary);

      service_monitor_->service_detected.connect([this](auto&& services) {
        if (!services->get_services().empty()) {
          close_connection();

          // Use first matched service.
          open_connection(services->get_services().front());
        }
      });

      service_monitor_->service_removed.connect([this](auto&& services) {
        if (!services->get_services().empty()) {
          close_connection();

          // Use the next service
          service_monitor_->async_invoke_service_detected();
        }
      });

      service_monitor_->async_start();

      CFRelease(matching_dictionary);
    }
  }

  virtual ~hid_system_client(void) {
    detach_from_dispatcher([this] {
      close_connection();

      service_monitor_ = nullptr;
    });
  }

  void async_set_caps_lock_state(bool state) {
    enqueue_to_dispatcher([this, state] {
      set_modifier_lock_state(kIOHIDCapsLockState, state);
    });
  }

private:
  // This method is executed in the dispatcher thread.
  void open_connection(io_service_t s) {
    service_ = s;
    IOObjectRetain(service_);

    auto kr = IOServiceOpen(service_, mach_task_self(), kIOHIDParamConnectType, &connect_);
    if (kr == KERN_SUCCESS) {
      logger::get_logger().info("IOServiceOpen is succeeded @ {0}", __PRETTY_FUNCTION__);

    } else {
      logger::get_logger().error("IOServiceOpen error: {1} @ {0}", __PRETTY_FUNCTION__, kr);
      connect_ = IO_OBJECT_NULL;
    }
  }

  // This method is executed in the dispatcher thread.
  void close_connection(void) {
    if (connect_) {
      auto kr = IOServiceClose(connect_);
      if (kr != kIOReturnSuccess) {
        logger::get_logger().error("IOConnectRelease error: {1} @ {0}", __PRETTY_FUNCTION__, kr);
      }
      connect_ = IO_OBJECT_NULL;
    }

    logger::get_logger().info("IOServiceClose is succeeded @ {0}", __PRETTY_FUNCTION__);

    if (service_) {
      IOObjectRelease(service_);
      service_ = IO_OBJECT_NULL;
    }
  }

  // This method is executed in the dispatcher thread.
  void set_modifier_lock_state(int selector, bool state) {
    if (!connect_) {
      logger::get_logger().error("connect_ is null @ {0}", __PRETTY_FUNCTION__);
      return;
    }

    auto kr = IOHIDSetModifierLockState(connect_, selector, state);
    if (kr != KERN_SUCCESS) {
      logger::get_logger().error("IOHIDSetModifierLockState error: {1} @ {0}", __PRETTY_FUNCTION__, kr);
    }
  }

  std::unique_ptr<monitor::service_monitor::monitor> service_monitor_;
  io_service_t service_;
  io_connect_t connect_;
};
} // namespace krbn
