#pragma once

// `krbn::input_source_monitor` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include "cf_utility.hpp"
#include "dispatcher.hpp"
#include "input_source_utility.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <Carbon/Carbon.h>
#include <boost/signals2.hpp>

namespace krbn {
class input_source_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  boost::signals2::signal<void(const input_source_identifiers& input_source_identifiers)> input_source_changed;

  // Methods

  input_source_monitor(const input_source_monitor&) = delete;

  input_source_monitor(void) : dispatcher_client(),
                               started_(false) {
  }

  virtual ~input_source_monitor(void) {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      if (started_) {
        logger::get_logger().warn("input_source_monitor is already started.");
        return;
      }

      started_ = true;

      CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(),
                                      this,
                                      static_input_source_changed_callback,
                                      kTISNotifySelectedKeyboardInputSourceChanged,
                                      nullptr,
                                      CFNotificationSuspensionBehaviorDeliverImmediately);

      logger::get_logger().info("input_source_monitor is started.");

      // Call `input_source_changed` slots for the current input source.

      input_source_changed_callback();
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

private:
  void stop(void) {
    if (!started_) {
      return;
    }

    CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(),
                                       this,
                                       kTISNotifySelectedKeyboardInputSourceChanged,
                                       nullptr);

    started_ = false;

    logger::get_logger().info("input_source_monitor is stopped.");
  }

  static void static_input_source_changed_callback(CFNotificationCenterRef center,
                                                   void* observer,
                                                   CFStringRef notification_name,
                                                   const void* observed_object,
                                                   CFDictionaryRef user_info) {
    auto self = static_cast<input_source_monitor*>(observer);
    if (self) {
      self->input_source_changed_callback();
    }
  }

  void input_source_changed_callback(void) {
    enqueue_to_dispatcher([this] {
      if (auto p = TISCopyCurrentKeyboardInputSource()) {
        input_source_identifiers identifiers(p);

        enqueue_to_dispatcher([this, identifiers] {
          input_source_changed(identifiers);
        });

        CFRelease(p);
      }
    });
  }

  bool started_;
};
} // namespace krbn
