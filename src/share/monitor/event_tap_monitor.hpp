#pragma once

// `krbn::event_tap_monitor` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include "dispatcher.hpp"
#include "event_tap_utility.hpp"
#include "logger.hpp"
#include <boost/signals2.hpp>
#include <pqrs/cf_run_loop_thread.hpp>

namespace krbn {
class event_tap_monitor final : pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  boost::signals2::signal<void(event_type, event_queue::event)> pointing_device_event_arrived;

  // Methods

  event_tap_monitor(const event_tap_monitor&) = delete;

  event_tap_monitor(void) : dispatcher_client(),
                            event_tap_(nullptr),
                            run_loop_source_(nullptr) {
    cf_run_loop_thread_ = std::make_unique<pqrs::cf_run_loop_thread>();
  }

  ~event_tap_monitor(void) {
    detach_from_dispatcher([this] {
      if (event_tap_) {
        CGEventTapEnable(event_tap_, false);

        if (run_loop_source_) {
          CFRunLoopRemoveSource(cf_run_loop_thread_->get_run_loop(),
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

    cf_run_loop_thread_->terminate();
    cf_run_loop_thread_ = nullptr;
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
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
          CFRunLoopAddSource(cf_run_loop_thread_->get_run_loop(),
                             run_loop_source_,
                             kCFRunLoopCommonModes);
          CGEventTapEnable(event_tap_, true);

          cf_run_loop_thread_->wake();

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
          enqueue_to_dispatcher([this, pair] {
            pointing_device_event_arrived(pair->first, pair->second);
          });
        }
        break;

      case kCGEventNull:
      case kCGEventKeyDown:
      case kCGEventKeyUp:
      case kCGEventFlagsChanged:
      case kCGEventTabletPointer:
      case kCGEventTabletProximity:
        // These events are not captured.
        break;
    }

    return event;
  }

  std::unique_ptr<pqrs::cf_run_loop_thread> cf_run_loop_thread_;
  CFMachPortRef _Nullable event_tap_;
  CFRunLoopSourceRef _Nullable run_loop_source_;
};
} // namespace krbn
