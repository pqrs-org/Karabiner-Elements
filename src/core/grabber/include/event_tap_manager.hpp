#pragma once

#include "gcd_utility.hpp"
#include "logger.hpp"
#include <CoreGraphics/CoreGraphics.h>

class event_tap_manager final {
public:
  typedef std::function<void(bool)> caps_lock_state_changed_callback;

  event_tap_manager(const event_tap_manager&) = delete;

  event_tap_manager(const caps_lock_state_changed_callback& caps_lock_state_changed_callback) : caps_lock_state_changed_callback_(caps_lock_state_changed_callback),
                                                                                                event_tap_(nullptr),
                                                                                                run_loop_source_(nullptr),
                                                                                                last_flags_(0) {
    // Observe flags changed in order to get the caps lock state.
    auto mask = CGEventMaskBit(kCGEventFlagsChanged);

    event_tap_ = CGEventTapCreate(kCGHIDEventTap,
                                  kCGHeadInsertEventTap,
                                  kCGEventTapOptionDefault,
                                  mask,
                                  event_tap_manager::static_callback,
                                  this);

    if (event_tap_) {
      run_loop_source_ = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, event_tap_, 0);
      if (run_loop_source_) {
        CFRunLoopAddSource(CFRunLoopGetMain(), run_loop_source_, kCFRunLoopCommonModes);
        CGEventTapEnable(event_tap_, true);

        logger::get_logger().info("event_tap_manager initialized");
      }
    }
  }

  ~event_tap_manager(void) {
    // Release event_tap_ in main thread to avoid callback invocations after object has been destroyed.
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (event_tap_) {
        CGEventTapEnable(event_tap_, false);

        if (run_loop_source_) {
          CFRunLoopRemoveSource(CFRunLoopGetMain(), run_loop_source_, kCFRunLoopCommonModes);
          CFRelease(run_loop_source_);
          run_loop_source_ = nullptr;
        }

        CFRelease(event_tap_);
        event_tap_ = nullptr;
      }
      logger::get_logger().info("event_tap_manager terminated");
    });
  }

  bool get_caps_lock_state(void) const {
    return (last_flags_ & NX_ALPHASHIFTMASK);
  }

private:
  static CGEventRef _Nullable static_callback(CGEventTapProxy _Nullable proxy, CGEventType type, CGEventRef _Nullable event, void* _Nonnull refcon) {
    auto self = static_cast<event_tap_manager*>(refcon);
    if (self) {
      return self->callback(proxy, type, event);
    }
    return nullptr;
  }

  CGEventRef _Nullable callback(CGEventTapProxy _Nullable proxy, CGEventType type, CGEventRef _Nullable event) {
    if (type == kCGEventTapDisabledByTimeout) {
      logger::get_logger().info("Re-enable event_tap_ by kCGEventTapDisabledByTimeout");
      CGEventTapEnable(event_tap_, true);
    } else if (type == kCGEventFlagsChanged) {
      // The caps lock key event from keyboard might be ignored by caps lock delay.
      // Thus, we need to observe kCGEventFlagsChanged event in EventTap to detect the caps lock state change.

      bool old_caps_lock_state = get_caps_lock_state();
      last_flags_ = CGEventGetFlags(event);
      bool new_caps_lock_state = get_caps_lock_state();

      if (old_caps_lock_state != new_caps_lock_state) {
        if (caps_lock_state_changed_callback_) {
          caps_lock_state_changed_callback_(new_caps_lock_state);
        }
      }
    }
    return event;
  }

  caps_lock_state_changed_callback caps_lock_state_changed_callback_;

  CFMachPortRef _Nullable event_tap_;
  CFRunLoopSourceRef _Nullable run_loop_source_;
  CGEventFlags last_flags_;
};
