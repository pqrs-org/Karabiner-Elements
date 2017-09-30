#pragma once

#include "boost_defs.hpp"

#include "gcd_utility.hpp"
#include "logger.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <boost/optional.hpp>

namespace krbn {
class event_tap_manager final {
public:
  typedef std::function<void(bool)> caps_lock_state_changed_callback;
  typedef std::function<void(CGEventType, CGEventRef _Nullable)> pointing_device_event_callback;

  event_tap_manager(const event_tap_manager&) = delete;

  event_tap_manager(const caps_lock_state_changed_callback& caps_lock_state_changed_callback,
                    const pointing_device_event_callback& pointing_device_event_callback) : caps_lock_state_changed_callback_(caps_lock_state_changed_callback),
                                                                                            pointing_device_event_callback_(pointing_device_event_callback),
                                                                                            event_tap_(nullptr),
                                                                                            run_loop_source_(nullptr) {
    // Observe flags changed in order to get the caps lock state and pointing device events from Apple trackpads.
    //
    // Note:
    // We cannot catch pointing device events from Apple trackpads without eventtap.
    // (Apple trackpad driver does not send events to IOKit.)

    auto mask = CGEventMaskBit(kCGEventFlagsChanged) |
                CGEventMaskBit(kCGEventLeftMouseDown) |
                CGEventMaskBit(kCGEventLeftMouseUp) |
                CGEventMaskBit(kCGEventRightMouseDown) |
                CGEventMaskBit(kCGEventRightMouseUp) |
                CGEventMaskBit(kCGEventMouseMoved) |
                CGEventMaskBit(kCGEventLeftMouseDragged) |
                CGEventMaskBit(kCGEventRightMouseDragged) |
                CGEventMaskBit(kCGEventScrollWheel) |
                CGEventMaskBit(kCGEventOtherMouseDown) |
                CGEventMaskBit(kCGEventOtherMouseUp) |
                CGEventMaskBit(kCGEventOtherMouseDragged);

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

  boost::optional<bool> get_caps_lock_state(void) const {
    if (last_flags_) {
      return (*last_flags_ & NX_ALPHASHIFTMASK);
    }
    return boost::none;
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
    switch (type) {
      case kCGEventTapDisabledByTimeout:
        logger::get_logger().info("Re-enable event_tap_ by kCGEventTapDisabledByTimeout");
        CGEventTapEnable(event_tap_, true);
        break;

      case kCGEventTapDisabledByUserInput:
        break;

      case kCGEventFlagsChanged:
        // The caps lock key event from keyboard might be ignored by caps lock delay.
        // Thus, we need to observe kCGEventFlagsChanged event in EventTap to detect the caps lock state change.
        if (event) {
          auto old_caps_lock_state = get_caps_lock_state();
          last_flags_ = CGEventGetFlags(event);
          auto new_caps_lock_state = get_caps_lock_state();

          if (old_caps_lock_state &&
              new_caps_lock_state &&
              *old_caps_lock_state == *new_caps_lock_state) {
            // the caps lock state is not changed.
          } else {
            if (caps_lock_state_changed_callback_) {
              caps_lock_state_changed_callback_(*new_caps_lock_state);
            }
          }
        }
        break;

      case kCGEventLeftMouseDown:
      case kCGEventLeftMouseUp:
      case kCGEventRightMouseDown:
      case kCGEventRightMouseUp:
      case kCGEventMouseMoved:
      case kCGEventLeftMouseDragged:
      case kCGEventRightMouseDragged:
      case kCGEventScrollWheel:
      case kCGEventOtherMouseDown:
      case kCGEventOtherMouseUp:
      case kCGEventOtherMouseDragged:
        if (pointing_device_event_callback_) {
          pointing_device_event_callback_(type, event);
        }
        break;

      case kCGEventNull:
      case kCGEventKeyDown:
      case kCGEventKeyUp:
      case kCGEventTabletPointer:
      case kCGEventTabletProximity:
        // These events are not captured.
        break;
    }

    return event;
  }

  caps_lock_state_changed_callback caps_lock_state_changed_callback_;
  pointing_device_event_callback pointing_device_event_callback_;

  CFMachPortRef _Nullable event_tap_;
  CFRunLoopSourceRef _Nullable run_loop_source_;
  boost::optional<CGEventFlags> last_flags_;
};
} // namespace krbn
