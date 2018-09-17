#pragma once

// `krbn::event_tap_monitor` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include "cf_utility.hpp"
#include "event_tap_utility.hpp"
#include "logger.hpp"
#include <boost/signals2.hpp>

namespace krbn {
class event_tap_monitor final {
public:
  // Signals

  boost::signals2::signal<void(bool)> caps_lock_state_changed;
  boost::signals2::signal<void(event_type, event_queue::entry::event)> pointing_device_event_arrived;

  // Methods

  event_tap_monitor(const event_tap_monitor&) = delete;

  event_tap_monitor(void) : event_tap_(nullptr),
                            run_loop_source_(nullptr) {
    run_loop_thread_ = std::make_unique<cf_utility::run_loop_thread>();
  }

  ~event_tap_monitor(void) {
    run_loop_thread_->enqueue(^{
      if (event_tap_) {
        CGEventTapEnable(event_tap_, false);

        if (run_loop_source_) {
          CFRunLoopRemoveSource(run_loop_thread_->get_run_loop(),
                                run_loop_source_,
                                kCFRunLoopCommonModes);
          CFRelease(run_loop_source_);
          run_loop_source_ = nullptr;
        }

        CFRelease(event_tap_);
        event_tap_ = nullptr;
      }
      logger::get_logger().info("event_tap_monitor terminated");
    });

    run_loop_thread_->terminate();
    run_loop_thread_ = nullptr;
  }

  void async_start(void) {
    run_loop_thread_->enqueue(^{
      if (event_tap_) {
        return;
      }

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
                                    kCGTailAppendEventTap,
                                    kCGEventTapOptionListenOnly,
                                    mask,
                                    event_tap_monitor::static_callback,
                                    this);

      if (event_tap_) {
        run_loop_source_ = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, event_tap_, 0);
        if (run_loop_source_) {
          CFRunLoopAddSource(run_loop_thread_->get_run_loop(),
                             run_loop_source_,
                             kCFRunLoopCommonModes);
          CGEventTapEnable(event_tap_, true);

          logger::get_logger().info("event_tap_monitor initialized");
        }
      }
    });
  }

private:
  static CGEventRef _Nullable static_callback(CGEventTapProxy _Nullable proxy, CGEventType type, CGEventRef _Nullable event, void* _Nonnull refcon) {
    auto self = static_cast<event_tap_monitor*>(refcon);
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
            caps_lock_state_changed(*new_caps_lock_state);
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
        if (auto pair = event_tap_utility::make_event(type, event)) {
          pointing_device_event_arrived(pair->first, pair->second);
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

  boost::optional<bool> get_caps_lock_state(void) const {
    if (last_flags_) {
      return (*last_flags_ & NX_ALPHASHIFTMASK);
    }
    return boost::none;
  }

  std::unique_ptr<cf_utility::run_loop_thread> run_loop_thread_;
  CFMachPortRef _Nullable event_tap_;
  CFRunLoopSourceRef _Nullable run_loop_source_;
  boost::optional<CGEventFlags> last_flags_;
};
} // namespace krbn
