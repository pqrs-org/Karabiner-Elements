#pragma once

#include "gcd_utility.hpp"
#include "logger.hpp"
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <functional>

namespace krbn {
class service_observer final {
public:
  typedef std::function<void(io_iterator_t iterator)> callback;

  service_observer(const service_observer&) = delete;

  service_observer(CFDictionaryRef _Nonnull matching_dictionary,
                   const callback& matched_callback,
                   const callback& terminated_callback) : matched_callback_(matched_callback),
                                                          terminated_callback_(terminated_callback),
                                                          notification_port_(nullptr),
                                                          matched_notification_(IO_OBJECT_NULL),
                                                          terminated_notification_(IO_OBJECT_NULL) {
    notification_port_ = IONotificationPortCreate(kIOMasterPortDefault);
    if (!notification_port_) {
      logger::get_logger().error("IONotificationPortCreate is failed @ {0}", __PRETTY_FUNCTION__);
      return;
    }

    if (auto loop_source = IONotificationPortGetRunLoopSource(notification_port_)) {
      CFRunLoopAddSource(CFRunLoopGetMain(), loop_source, kCFRunLoopDefaultMode);
    } else {
      logger::get_logger().error("IONotificationPortGetRunLoopSource is failed @ {0}", __PRETTY_FUNCTION__);
    }

    // kIOMatchedNotification
    {
      CFRetain(matching_dictionary);
      auto kr = IOServiceAddMatchingNotification(notification_port_,
                                                 kIOMatchedNotification,
                                                 matching_dictionary,
                                                 &(service_observer::static_matched_callback),
                                                 static_cast<void*>(this),
                                                 &matched_notification_);
      if (kr != kIOReturnSuccess) {
        logger::get_logger().error("IOServiceAddMatchingNotification error: {1} @ {0}", __PRETTY_FUNCTION__, kr);
        CFRelease(matching_dictionary);
      } else {
        matched_callback(matched_notification_);
      }
    }

    // kIOTerminatedNotification
    {
      CFRetain(matching_dictionary);
      auto kr = IOServiceAddMatchingNotification(notification_port_,
                                                 kIOTerminatedNotification,
                                                 matching_dictionary,
                                                 &(service_observer::static_terminated_callback),
                                                 static_cast<void*>(this),
                                                 &terminated_notification_);
      if (kr != kIOReturnSuccess) {
        logger::get_logger().error("IOServiceAddMatchingNotification error: {1} @ {0}", __PRETTY_FUNCTION__, kr);
        CFRelease(matching_dictionary);
      } else {
        terminated_callback(terminated_notification_);
      }
    }
  }

  ~service_observer(void) {
    // Release matched_notification_ and terminated_notification_ in main thread to avoid callback invocations after object has been destroyed.
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (matched_notification_) {
        IOObjectRelease(matched_notification_);
        matched_notification_ = IO_OBJECT_NULL;
      }

      if (terminated_notification_) {
        IOObjectRelease(terminated_notification_);
        terminated_notification_ = IO_OBJECT_NULL;
      }

      if (notification_port_) {
        if (auto loop_source = IONotificationPortGetRunLoopSource(notification_port_)) {
          CFRunLoopRemoveSource(CFRunLoopGetMain(), loop_source, kCFRunLoopDefaultMode);
        }

        IONotificationPortDestroy(notification_port_);
        notification_port_ = nullptr;
      }
    });
  }

private:
  static void static_matched_callback(void* _Nonnull refcon, io_iterator_t iterator) {
    service_observer* self = static_cast<service_observer*>(refcon);
    if (self->matched_callback_) {
      self->matched_callback_(iterator);
    }
  }

  static void static_terminated_callback(void* _Nonnull refcon, io_iterator_t iterator) {
    service_observer* self = static_cast<service_observer*>(refcon);
    if (self->terminated_callback_) {
      self->terminated_callback_(iterator);
    }
  }

  callback matched_callback_;
  callback terminated_callback_;

  IONotificationPortRef _Nullable notification_port_;
  io_iterator_t matched_notification_;
  io_iterator_t terminated_notification_;
};
} // namespace krbn
