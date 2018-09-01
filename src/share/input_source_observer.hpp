#pragma once

// `krbn::input_source_observer` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include "cf_utility.hpp"
#include "input_source_utility.hpp"
#include "logger.hpp"
#include "thread_utility.hpp"
#include "types.hpp"
#include <Carbon/Carbon.h>
#include <boost/signals2.hpp>

namespace krbn {
class input_source_observer final {
public:
  // Signals

  boost::signals2::signal<void(const input_source_identifiers& input_source_identifiers)> input_source_changed;

  // Methods

  input_source_observer(const input_source_observer&) = delete;

  input_source_observer(void) : started_(false) {
    dispatcher_ = std::make_unique<thread_utility::dispatcher>();
  }

  ~input_source_observer(void) {
    async_stop();

    dispatcher_->terminate();
    dispatcher_ = nullptr;
  }

  void async_start(void) {
    dispatcher_->enqueue([this] {
      if (started_) {
        logger::get_logger().warn("input_source_observer is already started.");
        return;
      }

      started_ = true;

      CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(),
                                      this,
                                      static_input_source_changed_callback,
                                      kTISNotifySelectedKeyboardInputSourceChanged,
                                      nullptr,
                                      CFNotificationSuspensionBehaviorDeliverImmediately);

      logger::get_logger().info("input_source_observer is started.");

      // Call `input_source_changed` slots for the current input source.

      input_source_changed_callback();
    });
  }

  void async_stop(void) {
    dispatcher_->enqueue([this] {
      if (!started_) {
        return;
      }

      CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(),
                                         this,
                                         kTISNotifySelectedKeyboardInputSourceChanged,
                                         nullptr);

      started_ = false;

      logger::get_logger().info("input_source_observer is stopped.");
    });
  }

private:
  static void static_input_source_changed_callback(CFNotificationCenterRef center,
                                                   void* observer,
                                                   CFStringRef notification_name,
                                                   const void* observed_object,
                                                   CFDictionaryRef user_info) {
    auto self = static_cast<input_source_observer*>(observer);
    if (self) {
      self->input_source_changed_callback();
    }
  }

  void input_source_changed_callback(void) {
    dispatcher_->enqueue([this] {
      if (auto p = TISCopyCurrentKeyboardInputSource()) {
        input_source_changed(input_source_identifiers(p));

        CFRelease(p);
      }
    });
  }

  std::unique_ptr<thread_utility::dispatcher> dispatcher_;
  bool started_;
};
} // namespace krbn
