#pragma once

#include "boost_defs.hpp"

#include "iokit_utility.hpp"
#include "logger.hpp"
#include "service_observer.hpp"
#include "types.hpp"
#include <boost/optional.hpp>
#include <list>
#include <memory>

namespace krbn {
class hid_system_client final {
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

  hid_system_client(void) : service_(IO_OBJECT_NULL),
                            connect_(IO_OBJECT_NULL) {
    matching_dictionary_ = IOServiceNameMatching(kIOHIDSystemClass);
    if (!matching_dictionary_) {
      logger::get_logger().error("IOServiceNameMatching error @ {0}", __PRETTY_FUNCTION__);
    } else {
      service_observer_ = std::make_unique<service_observer>(matching_dictionary_,
                                                             std::bind(&hid_system_client::matched_callback, this, std::placeholders::_1),
                                                             std::bind(&hid_system_client::terminated_callback, this, std::placeholders::_1));
    }
  }

  ~hid_system_client(void) {
    close_connection();

    if (matching_dictionary_) {
      CFRelease(matching_dictionary_);
    }
  }

  boost::optional<bool> get_caps_lock_state(void) {
    return get_modifier_lock_state(kIOHIDCapsLockState);
  }

  bool set_caps_lock_state(bool state) {
    return set_modifier_lock_state(kIOHIDCapsLockState, state);
  }

private:
  void matched_callback(io_iterator_t iterator) {
    while (auto service = IOIteratorNext(iterator)) {
      std::lock_guard<std::mutex> guard(connect_mutex_);

      // Use first matched service.
      if (!service_) {
        service_ = service;
        IOObjectRetain(service_);

        auto kr = IOServiceOpen(service_, mach_task_self(), kIOHIDParamConnectType, &connect_);
        if (kr != KERN_SUCCESS) {
          logger::get_logger().error("IOServiceOpen error: {1} @ {0}", __PRETTY_FUNCTION__, kr);
          connect_ = IO_OBJECT_NULL;
        }

        logger::get_logger().info("IOServiceOpen is succeeded @ {0}", __PRETTY_FUNCTION__);
      }

      IOObjectRelease(service);
    }
  }

  void terminated_callback(io_iterator_t iterator) {
    bool found = false;

    while (auto service = IOIteratorNext(iterator)) {
      found = true;
      IOObjectRelease(service);
    }

    if (!found) {
      return;
    }

    // ----------------------------------------
    // Refresh connection.

    close_connection();

    io_iterator_t it;
    auto kr = IOServiceGetMatchingServices(kIOMasterPortDefault, matching_dictionary_, &it);
    if (kr != KERN_SUCCESS) {
      logger::get_logger().error("IOServiceGetMatchingServices error: {1} @ {0}", __PRETTY_FUNCTION__, kr);
    } else {
      matched_callback(it);
      IOObjectRelease(it);
    }
  }

  boost::optional<bool> get_modifier_lock_state(int selector) {
    std::lock_guard<std::mutex> guard(connect_mutex_);

    if (!connect_) {
      logger::get_logger().error("connect_ is null @ {0}", __PRETTY_FUNCTION__);
      return boost::none;
    }

    bool value;
    auto kr = IOHIDGetModifierLockState(connect_, selector, &value);
    if (kr != KERN_SUCCESS) {
      logger::get_logger().error("IOHIDGetModifierLockState error: {1} @ {0}", __PRETTY_FUNCTION__, kr);
    }

    return value;
  }

  bool set_modifier_lock_state(int selector, bool state) {
    std::lock_guard<std::mutex> guard(connect_mutex_);

    if (!connect_) {
      logger::get_logger().error("connect_ is null @ {0}", __PRETTY_FUNCTION__);
      return false;
    }

    auto kr = IOHIDSetModifierLockState(connect_, selector, state);
    if (kr != KERN_SUCCESS) {
      logger::get_logger().error("IOHIDSetModifierLockState error: {1} @ {0}", __PRETTY_FUNCTION__, kr);
      return false;
    }

    return true;
  }

  void close_connection(void) {
    std::lock_guard<std::mutex> guard(connect_mutex_);

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

  std::unique_ptr<service_observer> service_observer_;
  CFMutableDictionaryRef _Nullable matching_dictionary_;
  io_service_t service_;
  io_connect_t connect_;
  std::mutex connect_mutex_;
};
} // namespace krbn
