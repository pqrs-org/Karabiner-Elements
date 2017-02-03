#pragma once

#include "constants.hpp"
#include "types.hpp"
#include <CoreFoundation/CoreFoundation.h>

class notification_center final {
public:
  static void post_distributed_notification(const char* _Nonnull name) {
    if (auto cfname = CFStringCreateWithCString(kCFAllocatorDefault,
                                                name,
                                                kCFStringEncodingUTF8)) {
      if (auto observed_object = CFStringCreateWithCString(kCFAllocatorDefault,
                                                           constants::get_distributed_notification_observed_object(),
                                                           kCFStringEncodingUTF8)) {
        CFNotificationCenterPostNotification(CFNotificationCenterGetDistributedCenter(),
                                             cfname,
                                             observed_object,
                                             nullptr,
                                             true);
        CFRelease(observed_object);
      }
      CFRelease(cfname);
    }
  }

  static void post_distributed_notification_to_all_sessions(const char* _Nonnull name) {
    if (auto cfname = CFStringCreateWithCString(kCFAllocatorDefault,
                                                name,
                                                kCFStringEncodingUTF8)) {
      if (auto observed_object = CFStringCreateWithCString(kCFAllocatorDefault,
                                                           constants::get_distributed_notification_observed_object(),
                                                           kCFStringEncodingUTF8)) {
        CFNotificationCenterPostNotificationWithOptions(CFNotificationCenterGetDistributedCenter(),
                                                        cfname,
                                                        observed_object,
                                                        nullptr,
                                                        kCFNotificationDeliverImmediately | kCFNotificationPostToAllSessions);
        CFRelease(observed_object);
      }
      CFRelease(cfname);
    }
  }

  static void observe_distributed_notification(const void* _Nullable observer,
                                               CFNotificationCallback _Nonnull callback,
                                               const char* _Nonnull name) {
    if (auto cfname = CFStringCreateWithCString(kCFAllocatorDefault,
                                                name,
                                                kCFStringEncodingUTF8)) {
      if (auto observed_object = CFStringCreateWithCString(kCFAllocatorDefault,
                                                           constants::get_distributed_notification_observed_object(),
                                                           kCFStringEncodingUTF8)) {
        CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(),
                                        observer,
                                        callback,
                                        cfname,
                                        observed_object,
                                        CFNotificationSuspensionBehaviorDeliverImmediately);
        CFRelease(observed_object);
      }
      CFRelease(cfname);
    }
  }
};
